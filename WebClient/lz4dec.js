//
// https://github.com/Benzinga/lz4js
//
// lz4.js - An implementation of Lz4 in plain JavaScript.
//

function LZ4() {

    // Compression format parameters/constants.
    var minMatch = 4;

    // Utility functions/primitives
    // --

    this.makeBuffer = function makeBuffer (size) {
	try {
	    return new Uint8ClampedArray(size);
	} catch (error) {
	    var buf = new Array(size);

	    for (var i = 0; i < size; i++) {
		buf[i] = 0;
	    }

	    return buf;
	}
    }

    function sliceArray (array, start, end) {
	if (typeof array.buffer !== undefined) {
	    if (Uint8ClampedArray.prototype.slice) {
		return array.slice(start, end);
	    } else {
		// Uint8ClampedArray#slice polyfill.
		var len = array.length;

		// Calculate start.
		start = start | 0;
		start = (start < 0) ? Math.max(len + start, 0) : Math.min(start, len);

		// Calculate end.
		end = (end === undefined) ? len : end | 0;
		end = (end < 0) ? Math.max(len + end, 0) : Math.min(end, len);

		// Copy into new array.
		var arraySlice = new Uint8ClampedArray(end - start);
		for (var i = start, n = 0; i < end;) {
		    arraySlice[n++] = array[i++];
		}

		return arraySlice;
	    }
	} else {
	    // Assume normal array.
	    return array.slice(start, end);
	}
    }

    // Decompresses a block of Lz4.
    this.decompressBlock = function decompressBlock (src, dst, sIndex, sLength, dIndex) {
	var mLength, mOffset, sEnd, n, i;
	var hasCopyWithin = dst.copyWithin !== undefined && dst.fill !== undefined;

	// Setup initial state.
	sEnd = sIndex + sLength;

	// Consume entire input block.
	while (sIndex < sEnd) {
	    var token = src[sIndex++];

	    // Copy literals.
	    var literalCount = (token >> 4);
	    if (literalCount > 0) {
		// Parse length.
		if (literalCount === 0xf) {
		    while (true) {
			literalCount += src[sIndex];
			if (src[sIndex++] !== 0xff) {
			    break;
			}
		    }
		}

		// Copy literals
		for (n = sIndex + literalCount; sIndex < n;) {
		    dst[dIndex++] = src[sIndex++];
		}
	    }

	    if (sIndex >= sEnd) {
		break;
	    }

	    // Copy match.
	    mLength = (token & 0xf);

	    // Parse offset.
	    mOffset = src[sIndex++] | (src[sIndex++] << 8);

	    // Parse length.
	    if (mLength === 0xf) {
		while (true) {
		    mLength += src[sIndex];
		    if (src[sIndex++] !== 0xff) {
			break;
		    }
		}
	    }

	    mLength += minMatch;

	    // Copy match
	    // prefer to use typedarray.copyWithin for larger matches
	    // NOTE: copyWithin doesn't work as required by LZ4 for overlapping sequences
	    // e.g. mOffset=1, mLength=30 (repeach char 30 times)
	    // we special case the repeat char w/ array.fill
	    if (hasCopyWithin && mOffset === 1) {
		dst.fill(dst[dIndex - 1] | 0, dIndex, dIndex + mLength);
		dIndex += mLength;
	    } else if (hasCopyWithin && mOffset > mLength && mLength > 31) {
		dst.copyWithin(dIndex, dIndex - mOffset, dIndex - mOffset + mLength);
		dIndex += mLength;
	    } else {
		for (i = dIndex - mOffset, n = i + mLength; i < n;) {
		    dst[dIndex++] = dst[i++] | 0;
		}
	    }
	}

	return dIndex;
    };

    this.decompress = function decompress(src, maxSize) {
	dst = this.makeBuffer(maxSize);

	size = this.decompressBlock(src, dst, 0, maxSize, 0);

	if (size !== maxSize) {
	    dst = sliceArray(dst, 0, size);
	}

	return dst;
    }
}

/*
function byteArray (arg) {
  if (Uint8ClampedArray) {
    return new Uint8ClampedArray(arg);
  } else {
    if (typeof arg === 'number' || typeof arg === 'undefined') {
      return new Array(arg);
    } else {
      return arg;
    }
  }
}

var input = byteArray([
0xf1, 0x0d, 0x48, 0x65, 0x6c, 0x6c, 0x6f, 0x20, 
0x77, 0x6f, 0x72, 0x6c, 0x64, 0x21, 0x20, 0x68, 
0x61, 0x68, 0x61, 0x2c, 0x20, 0x73, 0x6f, 0x6d, 
0x65, 0x20, 0x6c, 0x69, 0x6b, 0x65, 0x0a, 0x00, 
0x10, 0x21, 0x01, 0x00, 0x2b, 0x20, 0x41, 0x01, 
0x00, 0xb2, 0x21, 0x20, 0x2d, 0x2d, 0x2d, 0x3d, 
0x3d, 0x3d, 0x20, 0x48, 0x41, 0x02, 0x00, 0x80, 
0x20, 0x3d, 0x3d, 0x3d, 0x2d, 0x2d, 0x2d, 0x00, 
]);

var lz4 = new LZ4();

console.log(lz4.decompress(input, 80));
 */
