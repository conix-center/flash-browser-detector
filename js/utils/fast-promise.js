// adapted from: https://github.com/alemart/speedy-vision-js/blob/master/src/utils/speedy-promise.js
const PENDING = 0;
const FULFILLED = 1;
const REJECTED = 2;

const SUSPEND_ASYNC = 1;
const asap = (typeof queueMicrotask !== 'undefined' && queueMicrotask) || // browsers
             (typeof process !== 'undefined' && process.nextTick) || // node.js
             (f => Promise.resolve().then(f)); // most compatible

export class FastPromise {
    constructor(callback) {
        this._state = PENDING;
        this._value = undefined;

        this._onFulfillment = null;
        this._onRejection = null;
        this._children = 0;
        this[0] = this;
        this._parent = undefined;
        this._flags = 0;

        this._fulfill = this._fulfill.bind(this);
        this._reject = this._reject.bind(this);
        this._resolve = this._resolve.bind(this);
        this._broadcastIfAsync = this._broadcastIfAsync.bind(this);

        callback(this._fulfill, this._reject);
    }

    then(onFulfillment, onRejection = null) {
        const child = new FastPromise(this._nop);
        child._onFulfillment = typeof onFulfillment === 'function' && onFulfillment;
        child._onRejection = typeof onRejection === 'function' && onRejection;
        child._parent = this;

        this[this._children++] = child; // attach child
        this._flags &= ~SUSPEND_ASYNC; // restore the async behavior
        this._notify();

        return child;
    }

    catch(onRejection) {
        return this.then(null, onRejection);
    }

    finally(onFinally) {
        const fn = val => { onFinally(); return val; };
        return this.then(fn, fn);
    }

    turbocharge() {
        let my = this;

        // suspend the async behavior
        this._flags |= SUSPEND_ASYNC;
        while(my._parent !== undefined) {
            my = my._parent;
            my._flags |= SUSPEND_ASYNC;
        }

        // notify the children of the root
        my._notify(); // will be synchronous

        // return this FastPromise
        return this;
    }

    toString() {
        switch(this._state) {
            case PENDING:
                return `FastPromise { <pending> }`;
            case FULFILLED:
                return `FastPromise { <fulfilled> ${this._value} }`;
            case REJECTED:
                return `FastPromise { <rejected> ${this._value} }`;
            default:
                return '';
        }
    }

    static resolve(value) {
        const promise = new FastPromise(this._snop);

        if((typeof value === 'object' && value !== null && 'then' in value) || (typeof value === 'function' && 'then' in value)) {
            // resolve asynchronously
            promise._resolve(value);
        }
        else {
            // fulfill synchronously
            promise._value = value;
            promise._state = FULFILLED;
        }

        return promise;
    }

    static reject(reason) {
        const promise = new FastPromise(this._snop);
        promise._value = reason;
        promise._state = REJECTED;
        return promise;
    }

    static all(iterable) {
        return new FastPromise((resolve, reject) => {
            const input = [];

            // get elements
            for(const element of iterable)
                input.push(element);

            // resolve synchronously if there are no elements
            const length = input.length;
            if(length == 0) {
                resolve([]);
                return;
            }

            // resolve asynchronously
            let counter = length;
            const output = new Array(length);
            const partialResolve = i => (val => { output[i] = val; if(0 == --counter) resolve(output); });
            for(let i = 0; i < length; i++) {
                const element = input[i];
                if(element.__proto__ === FastPromise.prototype || element.__proto__ === Promise.prototype)
                    element.then(partialResolve(i), reject);
                else
                    FastPromise.resolve(element).then(partialResolve(i), reject);
            }
        });
    }

    static race(iterable) {
        return new FastPromise((resolve, reject) => {
            const input = [];

            // get elements
            for(const element of iterable)
                input.push(element);

            // if the iterable is empty, the promise
            // will be pending forever...

            // resolve asynchronously
            const length = input.length;
            for(let i = 0; i < length; i++) {
                const element = input[i];
                if(element.__proto__ === FastPromise.prototype || element.__proto__ === Promise.prototype)
                    element.then(resolve, reject);
                else
                    FastPromise.resolve(element).then(resolve, reject);
            }
        });
    }

    _fulfill(value) {
        this._setState(FULFILLED, value);
    }

    _reject(reason) {
        this._setState(REJECTED, reason);
    }

    _setState(state, value) {
        // the promise is already fulfilled or rejected
        if(this._state != PENDING)
            return;

        // set the new state
        this._state = state;
        this._value = value;
        this._notify();
    }

    _notify() {
        // nothing to do
        if(this._state == PENDING)
            return;

        // have we turbocharged this promise?
        if(this._flags & SUSPEND_ASYNC) {
            this._broadcast(); // execute synchronously
            return;
        }

        // install a timer (default behavior)
        asap(this._broadcastIfAsync);
    }

    _broadcastIfAsync() {
        // we may have installed a timer at some
        // point, but turbocharged the promise later
        if(!(this._flags & SUSPEND_ASYNC))
            this._broadcast();
    }

    _broadcast() {
        const children = this._children;
        const state = this._state;

        if(state === FULFILLED) {
            for(let i = 0; i < children; i++) {
                const child = this[i];
                const callback = child._onFulfillment;

                try {
                    if(callback) {
                        if(callback !== child._nop) {
                            child._resolve(callback(this._value)); // promise resolution procedure
                            child._onFulfillment = child._nop; // will not be called again
                        }
                    }
                    else
                        child._fulfill(this._value);
                }
                catch(e) {
                    child._reject(e);
                }
            }
        }
        else if(state === REJECTED) {
            for(let i = 0; i < children; i++) {
                const child = this[i];
                const callback = child._onRejection;

                try {
                    if(callback) {
                        if(callback !== child._nop) {
                            child._resolve(callback(this._value)); // promise resolution procedure
                            child._onRejection = child._nop; // will not be called again
                        }
                    }
                    else
                        child._reject(this._value);
                }
                catch(e) {
                    child._reject(e);
                }
            }
        }
    }

    _resolve(x) {
        if((typeof x !== 'object' && typeof x !== 'function') || (x === null)) { // if(x !== Object(x))
            this._fulfill(x);
            return;
        }

        if(x === this)
            throw new TypeError(); // Circular reference

        if(x.__proto__ === FastPromise.prototype || x.__proto__ === Promise.prototype) {
            x.then(this._resolve, this._reject);
            return;
        }

        try {
            const then = x.then;
            if(typeof then === 'function') {
                let resolve = this._resolve, reject = this._reject;
                try {
                    then.call(x,
                        y => { resolve(y); resolve = reject = this._nop; },
                        r => { reject(r); resolve = reject = this._nop; }
                    );
                }
                catch(e) {
                    if(resolve !== this._nop && reject !== this._nop)
                        this._reject(e);
                }
            }
            else {
                this._fulfill(x);
            }
        }
        catch(e) {
            this._reject(e);
        }
    }

    _nop() {
    }

    static _snop() {
    }
}
