/** @file lightanchor_detector.c
 *  @brief Implementation of the lightanchor detector library
 *  @see lightanchor_detector.h for documentation
 *
 * Copyright (C) Wiselab CMU.
 * @date July, 2020
 *
 */
#include <math.h>

#include "common/zarray.h"
#include "common/homography.h"
#include "common/g2d.h"
#include "common/math_util.h"
#include "common/matd.h"
#include "apriltag_math.h"

#include "apriltag.h"

#include "lightanchor.h"
#include "lightanchor_detector.h"
#include "decoder.h"
#include "queue_buf.h"

struct graymodel
{
    double A[3][3];
    double B[3];
    double C[3];
};

extern void graymodel_init(struct graymodel *gm);
extern void graymodel_add(struct graymodel *gm, double x, double y, double gray);
extern void graymodel_solve(struct graymodel *gm);
extern double graymodel_interpolate(struct graymodel *gm, double x, double y);

apriltag_family_t *lightanchor_family_create()
{
    apriltag_family_t *tf = calloc(1, sizeof(apriltag_family_t));
    if (tf == NULL)
        return NULL;

    tf->name = strdup("lightanchor");

    // same as tag36h11
    tf->width_at_border = 8;
    tf->total_width = 10;
    tf->reversed_border = false;

    return tf;
}

void lightanchor_family_destroy(apriltag_family_t *lf) {
    free(lf->name);
    free(lf);
}

lightanchor_detector_t *lightanchor_detector_create()
{
    lightanchor_detector_t *ld =
        (lightanchor_detector_t *)calloc(1, sizeof(lightanchor_detector_t));

    ld->candidates = zarray_create(sizeof(lightanchor_t *));
    ld->codes = zarray_create(sizeof(glitter_code_t));

    return ld;
}

int lightanchor_detector_add_code(lightanchor_detector_t *ld, char code)
{
    glitter_code_t glitter_code;
    glitter_code.code = code;
    glitter_code.doubled_code = double_bits(code);

    zarray_add(ld->codes, &glitter_code);

    return 0;
}

void lightanchor_detector_destroy(lightanchor_detector_t *ld)
{
    lightanchors_destroy(ld->candidates);
    zarray_destroy(ld->codes);
    free(ld);
}

static void refine_edges(apriltag_detector_t *td,
                         image_u8_t *im_orig, struct quad *quad)
{
    double lines[4][4]; // for each line, [Ex Ey nx ny]

    for (int edge = 0; edge < 4; edge++) {
        int a = edge, b = (edge + 1) & 3; // indices of the end points.

        // compute the normal to the current line estimate
        double nx = quad->p[b][1] - quad->p[a][1];
        double ny = -quad->p[b][0] + quad->p[a][0];
        double mag = sqrt(nx*nx + ny*ny);
        nx /= mag;
        ny /= mag;

        if (quad->reversed_border) {
            nx = -nx;
            ny = -ny;
        }

        // we will now fit a NEW line by sampling points near
        // our original line that have large gradients. On really big tags,
        // we're willing to sample more to get an even better estimate.
        int nsamples = imax(16, mag / 8); // XXX tunable

        // stats for fitting a line...
        double Mx = 0, My = 0, Mxx = 0, Mxy = 0, Myy = 0, N = 0;

        for (int s = 0; s < nsamples; s++) {
            // compute a point along the line... Note, we're avoiding
            // sampling *right* at the corners, since those points are
            // the least reliable.
            double alpha = (1.0 + s) / (nsamples + 1);
            double x0 = alpha*quad->p[a][0] + (1-alpha)*quad->p[b][0];
            double y0 = alpha*quad->p[a][1] + (1-alpha)*quad->p[b][1];

            // search along the normal to this line, looking at the
            // gradients along the way. We're looking for a strong
            // response.
            double Mn = 0;
            double Mcount = 0;

            // XXX tunable: how far to search?  We want to search far
            // enough that we find the best edge, but not so far that
            // we hit other edges that aren't part of the tag. We
            // shouldn't ever have to search more than quad_decimate,
            // since otherwise we would (ideally) have started our
            // search on another pixel in the first place. Likewise,
            // for very small tags, we don't want the range to be too
            // big.
            double range = td->quad_decimate + 1;

            // XXX tunable step size.
            for (double n = -range; n <= range; n +=  0.25) {
                // Because of the guaranteed winding order of the
                // points in the quad, we will start inside the white
                // portion of the quad and work our way outward.
                //
                // sample to points (x1,y1) and (x2,y2) XXX tunable:
                // how far +/- to look? Small values compute the
                // gradient more precisely, but are more sensitive to
                // noise.
                double grange = 1;
                int x1 = x0 + (n + grange)*nx;
                int y1 = y0 + (n + grange)*ny;
                if (x1 < 0 || x1 >= im_orig->width || y1 < 0 || y1 >= im_orig->height)
                    continue;

                int x2 = x0 + (n - grange)*nx;
                int y2 = y0 + (n - grange)*ny;
                if (x2 < 0 || x2 >= im_orig->width || y2 < 0 || y2 >= im_orig->height)
                    continue;

                int g1 = im_orig->buf[y1*im_orig->stride + x1];
                int g2 = im_orig->buf[y2*im_orig->stride + x2];

                if (g1 < g2) // reject points whose gradient is "backwards". They can only hurt us.
                    continue;

                double weight = (g2 - g1)*(g2 - g1); // XXX tunable. What shape for weight=f(g2-g1)?

                // compute weighted average of the gradient at this point.
                Mn += weight*n;
                Mcount += weight;
            }

            // what was the average point along the line?
            if (Mcount == 0)
                continue;

            double n0 = Mn / Mcount;

            // where is the point along the line?
            double bestx = x0 + n0*nx;
            double besty = y0 + n0*ny;

            // update our line fit statistics
            Mx += bestx;
            My += besty;
            Mxx += bestx*bestx;
            Mxy += bestx*besty;
            Myy += besty*besty;
            N++;
        }

        // fit a line
        double Ex = Mx / N, Ey = My / N;
        double Cxx = Mxx / N - Ex*Ex;
        double Cxy = Mxy / N - Ex*Ey;
        double Cyy = Myy / N - Ey*Ey;

        // TODO: Can replace this with same code as in fit_line.
        double normal_theta = .5 * atan2f(-2*Cxy, (Cyy - Cxx));
        nx = cosf(normal_theta);
        ny = sinf(normal_theta);
        lines[edge][0] = Ex;
        lines[edge][1] = Ey;
        lines[edge][2] = nx;
        lines[edge][3] = ny;
    }

    // now refit the corners of the quad
    for (int i = 0; i < 4; i++) {

        // solve for the intersection of lines (i) and (i+1)&3.
        double A00 =  lines[i][3],  A01 = -lines[(i+1)&3][3];
        double A10 =  -lines[i][2],  A11 = lines[(i+1)&3][2];
        double B0 = -lines[i][0] + lines[(i+1)&3][0];
        double B1 = -lines[i][1] + lines[(i+1)&3][1];

        double det = A00 * A11 - A10 * A01;

        // inverse.
        if (fabs(det) > 0.001) {
            // solve
            double W00 = A11 / det, W01 = -A01 / det;

            double L0 = W00*B0 + W01*B1;

            // compute intersection
            quad->p[i][0] = lines[i][0] + L0*A00;
            quad->p[i][1] = lines[i][1] + L0*A10;
        } else {
            // this is a bad sign. We'll just keep the corner we had.
            // printf("bad det: %15f %15f %15f %15f %15f\n", A00, A11, A10, A01, det);
        }
    }
}

zarray_t *detect_quads(apriltag_detector_t *td, image_u8_t *im_orig)
{
    if (td->wp == NULL || td->nthreads != workerpool_get_nthreads(td->wp))
    {
        workerpool_destroy(td->wp);
        td->wp = workerpool_create(td->nthreads);
    }

    image_u8_t *quad_im = image_u8_copy(im_orig);
    zarray_t *quads = apriltag_quad_thresh(td, quad_im);

    if (quad_im != im_orig)
        image_u8_destroy(quad_im);

    return quads;
}

static zarray_t *update_candidates(lightanchor_detector_t *ld,
                                   zarray_t *new_tags, image_u8_t *im)
{
    zarray_t *detections = zarray_create(sizeof(lightanchor_t *));

    if (zarray_size(ld->candidates) == 0)
    {
        for (int i = 0; i < zarray_size(new_tags); i++)
        {
            lightanchor_t *candidate;
            zarray_get(new_tags, i, &candidate);
            zarray_add(ld->candidates, &candidate);
        }
        zarray_destroy(new_tags);
    }
    else {
        for (int i = 0; i < zarray_size(ld->candidates); i++)
        {
            lightanchor_t *old_tag, *match_tag = NULL;
            zarray_get(ld->candidates, i, &old_tag);

            double dist, dist_shape = -1;
            double min_dist = MAX_DIST, min_dist_shape = MAX_DIST;
            // search for closest tag
            for (int j = 0; j < zarray_size(new_tags); j++)
            {
                lightanchor_t *new_tag;
                zarray_get(new_tags, j, &new_tag);

                // double dist = ( g2d_distance(old_tag->p[0], new_tag->p[0]) +
                //                 g2d_distance(old_tag->p[1], new_tag->p[1]) +
                //                 g2d_distance(old_tag->p[2], new_tag->p[2]) +
                //                 g2d_distance(old_tag->p[3], new_tag->p[3]) ) / 4;
                dist = g2d_distance(old_tag->c, new_tag->c);

                // reject tags with dissimilar shape
                // shape is represented as the average distance from each corner to the center
                // not scale invariant!
                double dist_shape_new = (g2d_distance(new_tag->p[0], new_tag->c) +
                                         g2d_distance(new_tag->p[1], new_tag->c) +
                                         g2d_distance(new_tag->p[2], new_tag->c) +
                                         g2d_distance(new_tag->p[3], new_tag->c)) / 4;
                double dist_shape_old = (g2d_distance(old_tag->p[0], old_tag->c) +
                                         g2d_distance(old_tag->p[1], old_tag->c) +
                                         g2d_distance(old_tag->p[2], old_tag->c) +
                                         g2d_distance(old_tag->p[3], old_tag->c)) / 4;
                dist_shape = fabs(dist_shape_new - dist_shape_old);

                if ((dist < min_dist) && (dist_shape < min_dist_shape) &&
                    (dist < ld->thres_dist_center) && (dist_shape < ld->thres_dist_shape))
                {
                    min_dist = dist;
                    min_dist_shape = dist_shape;
                    match_tag = new_tag;
                }
            }

            if (match_tag != NULL)
            {
                // only the closest match_tag can be matched with a prev tag
                if (match_tag->min_dist == 0 || min_dist < match_tag->min_dist)
                {
                    lightanchor_update(old_tag, match_tag);
                    match_tag->min_dist = min_dist;
                }
            }
            // stricter shape distance threshold for tags that have a ttl
            else if ((old_tag->frames > 0) &&
                     (dist_shape != -1) &&
                     (dist_shape < ld->thres_dist_shape_ttl)) {
                old_tag->frames--;
                zarray_add(new_tags, &old_tag);
                zarray_remove_index(ld->candidates, i, 1);
                i--;
            }
        }

        for (int i = 0; i < zarray_size(new_tags); i++)
        {
            lightanchor_t *candidate_curr;
            zarray_get(new_tags, i, &candidate_curr);

            uint8_t max, min, mean;
            uint8_t brightness = lightanchor_intensity(candidate_curr, im);
            qb_add(&candidate_curr->brightnesses, brightness);
            qb_stats(&candidate_curr->brightnesses, &max, &min, &mean);

            if (qb_full(&candidate_curr->brightnesses) && ((max - min) > ld->range_thres))
            {
                candidate_curr->code = (candidate_curr->code << 1) | (brightness > mean);
                candidate_curr->frames = ld->ttl_frames;

                if (lightanchor_decode(ld, candidate_curr)) {
                    lightanchor_t *det = lightanchor_copy(candidate_curr);

                    // double theta = 2.0 * M_PI / 2.0;
                    // double c = cos(theta), s = sin(theta);

                    // // Fix the rotation of our homography to properly orient the tag
                    // matd_t *R = matd_create(3,3);
                    // MATD_EL(R, 0, 0) = c;
                    // MATD_EL(R, 0, 1) = -s;
                    // MATD_EL(R, 1, 0) = s;
                    // MATD_EL(R, 1, 1) = c;
                    // MATD_EL(R, 2, 2) = 1;

                    // det->H = matd_op("M*M", det->H, R);

                    // matd_destroy(R);

                    homography_project(det->H, 0, 0, &det->c[0], &det->c[1]);

                    // [-1, -1], [1, -1], [1, 1], [-1, 1], Desired points
                    // [-1, 1], [1, 1], [1, -1], [-1, -1], FLIP Y
                    // adjust the points in det->p so that they correspond to
                    // counter-clockwise around the quad, starting at -1,-1.
                    for (int i = 0; i < 4; i++) {
                        int tcx = (i == 1 || i == 2) ? 1 : -1;
                        int tcy = (i < 2) ? 1 : -1;

                        double p[2];

                        homography_project(det->H, tcx, tcy, &p[0], &p[1]);

                        det->p[i][0] = p[0];
                        det->p[i][1] = p[1];
                    }

                    zarray_add(detections, &det);
                }
            }
        }

        lightanchors_destroy(ld->candidates);
        ld->candidates = new_tags;
    }

    return detections;
}

int quad_verify(apriltag_detector_t* td, apriltag_family_t *family, image_u8_t *im, struct quad *quad) {
    float patterns[] = {
        // left white column
        -0.5, 0.5,
        0, 1,
        1,

        // left black column
        0.5, 0.5,
        0, 1,
        0,

        // right white column
        family->width_at_border + 0.5, .5,
        0, 1,
        1,

        // right black column
        family->width_at_border - 0.5, .5,
        0, 1,
        0,

        // top white row
        0.5, -0.5,
        1, 0,
        1,

        // top black row
        0.5, 0.5,
        1, 0,
        0,

        // bottom white row
        0.5, family->width_at_border + 0.5,
        1, 0,
        1,

        // bottom black row
        0.5, family->width_at_border - 0.5,
        1, 0,
        0

        // XXX double-counts the corners.
    };

    struct graymodel whitemodel, blackmodel;
    graymodel_init(&whitemodel);
    graymodel_init(&blackmodel);

    for (int pattern_idx = 0; pattern_idx < sizeof(patterns)/(5*sizeof(float)); pattern_idx ++) {
        float *pattern = &patterns[pattern_idx * 5];

        int is_white = pattern[4];

        for (int i = 0; i < family->width_at_border; i++) {
            double tagx01 = (pattern[0] + i*pattern[2]) / (family->width_at_border);
            double tagy01 = (pattern[1] + i*pattern[3]) / (family->width_at_border);

            double tagx = 2*(tagx01-0.5);
            double tagy = 2*(tagy01-0.5);

            double px, py;
            homography_project(quad->H, tagx, tagy, &px, &py);

            // don't round
            int ix = px;
            int iy = py;
            if (ix < 0 || iy < 0 || ix >= im->width || iy >= im->height)
                continue;

            int v = im->buf[iy*im->stride + ix];

            if (is_white)
                graymodel_add(&whitemodel, tagx, tagy, v);
            else
                graymodel_add(&blackmodel, tagx, tagy, v);
        }
    }

    if (family->width_at_border > 1) {
        graymodel_solve(&whitemodel);
        graymodel_solve(&blackmodel);
    } else {
        graymodel_solve(&whitemodel);
        blackmodel.C[0] = 0;
        blackmodel.C[1] = 0;
        blackmodel.C[2] = blackmodel.B[2]/4;
    }

    if ((graymodel_interpolate(&whitemodel, 0, 0) - graymodel_interpolate(&blackmodel, 0, 0) < 0) != family->reversed_border) {
        return -1;
    }

    return 0;
}

zarray_t *decode_tags(apriltag_detector_t *td, lightanchor_detector_t *ld,
                      zarray_t *quads, image_u8_t *im)
{
    zarray_t *new_tags = zarray_create(sizeof(lightanchor_t *));

    for (int quadidx = 0; quadidx < zarray_size(quads); quadidx++)
    {
        struct quad *quad;
        zarray_get_volatile(quads, quadidx, &quad);

        // refine edges is not dependent upon the tag family, thus
        // apply this optimization BEFORE the other work.
        if (td->refine_edges)
            refine_edges(td, im, quad);

        // make sure the homographies are computed...
        if (quad_update_homographies(quad))
            continue;

        int status = 0;
        for (int famidx = 0; famidx < zarray_size(td->tag_families); famidx++) {
            apriltag_family_t *family;
            zarray_get(td->tag_families, famidx, &family);

            if (family->reversed_border != quad->reversed_border) {
                continue;
            }

            if ((status = quad_verify(td, family, im, quad)))
                break;
        }

        if (status)
            continue;

        lightanchor_t *lightanchor;
        if ((lightanchor = lightanchor_create(quad)) != NULL)
            zarray_add(new_tags, &lightanchor);
    }
    quads_destroy(quads);

    // return new_tags;
    return update_candidates(ld, new_tags, im);
}

zarray_t *lightanchor_detector_detect(apriltag_detector_t *td, lightanchor_detector_t *ld, image_u8_t *im)
{
    zarray_t *quads = detect_quads(td, im);
    return decode_tags(td, ld, quads, im);
}
