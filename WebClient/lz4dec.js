// lz4.js - An implementation of Lz4 in plain JavaScript.
//
// TODO:
// - Unify header parsing/writing.
// - Support options (block size, checksums)
// - Support streams
// - Better error handling (handle bad offset, etc.)
// - HC support (better search algorithm)
// - Tests/benchmarking

function Util() {
// Simple hash function, from: http://burtleburtle.net/bob/hash/integer.html.
// Chosen because it doesn't use multiply and achieves full avalanche.
    this.hashU32 = function hashU32(a) {
	a = a | 0;
	a = a + 2127912214 + (a << 12) | 0;
	a = a ^ -949894596 ^ a >>> 19;
	a = a + 374761393 + (a << 5) | 0;
	a = a + -744332180 ^ a << 9;
	a = a + -42973499 + (a << 3) | 0;
	return a ^ -1252372727 ^ a >>> 16 | 0;
    }

// Reads a 64-bit little-endian integer from an array.
    this.readU64 = function readU64(b, n) {
	var x = 0;
	x |= b[n++] << 0;
	x |= b[n++] << 8;
	x |= b[n++] << 16;
	x |= b[n++] << 24;
	x |= b[n++] << 32;
	x |= b[n++] << 40;
	x |= b[n++] << 48;
	x |= b[n++] << 56;
	return x;
    }

// Reads a 32-bit little-endian integer from an array.
    this.readU32 = function readU32(b, n) {
	var x = 0;
	x |= b[n++] << 0;
	x |= b[n++] << 8;
	x |= b[n++] << 16;
	x |= b[n++] << 24;
	return x;
    }

// Writes a 32-bit little-endian integer from an array.
    this.writeU32 = function writeU32(b, n, x) {
	b[n++] = (x >> 0) & 0xff;
	b[n++] = (x >> 8) & 0xff;
	b[n++] = (x >> 16) & 0xff;
	b[n++] = (x >> 24) & 0xff;
    }

// Multiplies two numbers using 32-bit integer multiplication.
// Algorithm from Emscripten.
    this.imul = function imul(a, b) {
	var ah = a >>> 16;
	var al = a & 65535;
	var bh = b >>> 16;
	var bl = b & 65535;

	return al * bl + (ah * bl + al * bh << 16) | 0;
    }

    // Makes a byte buffer. On older browsers, may return a plain array.
    this.makeBuffer = function makeBuffer (size) {
	try {
	    return new Uint8Array(size);
	} catch (error) {
	    var buf = new Array(size);

	    for (var i = 0; i < size; i++) {
		buf[i] = 0;
	    }

	    return buf;
	}
    }
}

function LZ4() {

    var util = new Util();

    // Constants
    // --

    // Compression format parameters/constants.
    var minMatch = 4;

    // Shared buffers
    var blockBuf = util.makeBuffer(5 << 20);

    // Frame constants.
    var magicNum = 0x184D2204;

    // Frame descriptor flags.
    var fdContentChksum = 0x4;
    var fdContentSize = 0x8;
    var fdBlockChksum = 0x10;
    // var fdBlockIndep = 0x20;
    var fdVersion = 0x40;
    var fdVersionMask = 0xC0;

    // Block sizes.
    var bsUncompressed = 0x80000000;
    var bsShift = 4;
    var bsMask = 7;
    var bsMap = {
	4: 0x10000,
	5: 0x40000,
	6: 0x100000,
	7: 0x400000
    };

    // Utility functions/primitives
    // --

    function sliceArray (array, start, end) {
	if (typeof array.buffer !== undefined) {
	    if (Uint8Array.prototype.slice) {
		return array.slice(start, end);
	    } else {
		// Uint8Array#slice polyfill.
		var len = array.length;

		// Calculate start.
		start = start | 0;
		start = (start < 0) ? Math.max(len + start, 0) : Math.min(start, len);

		// Calculate end.
		end = (end === undefined) ? len : end | 0;
		end = (end < 0) ? Math.max(len + end, 0) : Math.min(end, len);

		// Copy into new array.
		var arraySlice = new Uint8Array(end - start);
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

    // Implementation
    // --

    // Calculates an upper bound for lz4 decompression, by reading the data.
    this.decompressBound = function decompressBound (src) {
	var sIndex = 0;

	// Read magic number
	if (util.readU32(src, sIndex) !== magicNum) {
	    throw new Error('invalid magic number');
	}

	sIndex += 4;

	// Read descriptor
	var descriptor = src[sIndex++];

	// Check version
	if ((descriptor & fdVersionMask) !== fdVersion) {
	    throw new Error('incompatible descriptor version ' + (descriptor & fdVersionMask));
	}

	// Read flags
	var useBlockSum = (descriptor & fdBlockChksum) !== 0;
	var useContentSize = (descriptor & fdContentSize) !== 0;

	// Read block size
	var bsIdx = (src[sIndex++] >> bsShift) & bsMask;

	if (bsMap[bsIdx] === undefined) {
	    throw new Error('invalid block size ' + bsIdx);
	}

	var maxBlockSize = bsMap[bsIdx];

	// Get content size
	if (useContentSize) {
	    return util.readU64(src, sIndex);
	}

	// Checksum
	sIndex++;

	// Read blocks.
	var maxSize = 0;
	while (true) {
	    var blockSize = util.readU32(src, sIndex);
	    sIndex += 4;

	    if (blockSize & bsUncompressed) {
		blockSize &= ~bsUncompressed;
		maxSize += blockSize;
	    } else if (blockSize > 0) {
		maxSize += maxBlockSize;
	    }

	    if (blockSize === 0) {
		return maxSize;
	    }

	    if (useBlockSum) {
		sIndex += 4;
	    }

	    sIndex += blockSize;
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

    // Decompresses a frame of Lz4 data.
    this.decompressFrame = function decompressFrame (src, dst) {
	var useBlockSum, useContentSum, useContentSize, descriptor;
	var sIndex = 0;
	var dIndex = 0;

	// Read magic number
	if (util.readU32(src, sIndex) !== magicNum) {
	    throw new Error('invalid magic number');
	}

	sIndex += 4;

	// Read descriptor
	descriptor = src[sIndex++];

	// Check version
	if ((descriptor & fdVersionMask) !== fdVersion) {
	    throw new Error('incompatible descriptor version');
	}

	// Read flags
	useBlockSum = (descriptor & fdBlockChksum) !== 0;
	useContentSum = (descriptor & fdContentChksum) !== 0;
	useContentSize = (descriptor & fdContentSize) !== 0;

	// Read block size
	var bsIdx = (src[sIndex++] >> bsShift) & bsMask;

	if (bsMap[bsIdx] === undefined) {
	    throw new Error('invalid block size');
	}

	if (useContentSize) {
	    // TODO: read content size
	    sIndex += 8;
	}

	sIndex++;

	// Read blocks.
	while (true) {
	    var compSize;

	    compSize = util.readU32(src, sIndex);
	    sIndex += 4;

	    if (compSize === 0) {
		break;
	    }

	    if (useBlockSum) {
		// TODO: read block checksum
		sIndex += 4;
	    }

	    // Check if block is compressed
	    if ((compSize & bsUncompressed) !== 0) {
		// Mask off the 'uncompressed' bit
		compSize &= ~bsUncompressed;

		// Copy uncompressed data into destination buffer.
		for (var j = 0; j < compSize; j++) {
		    dst[dIndex++] = src[sIndex++];
		}
	    } else {
		// Decompress into blockBuf
		dIndex = exports.decompressBlock(src, dst, sIndex, compSize, dIndex);
		sIndex += compSize;
	    }
	}

	if (useContentSum) {
	    // TODO: read content checksum
	    sIndex += 4;
	}

	return dIndex;
    };

    // Decompresses a buffer containing an Lz4 frame. maxSize is optional; if not
    // provided, a maximum size will be determined by examining the data. The
    // buffer returned will always be perfectly-sized.
    this.decompress = function decompress (src, maxSize) {
	var dst, size;

	if (maxSize === undefined) {
	    maxSize = this.decompressBound(src);
	}
	dst = util.makeBuffer(maxSize);
	size = this.decompressFrame(src, dst);

	if (size !== maxSize) {
	    dst = sliceArray(dst, 0, size);
	}

	return dst;
    };
}

/*
var lz4 = new LZ4();

function byteArray (arg) {
  if (Uint8Array) {
    return new Uint8Array(arg);
  } else {
    if (typeof arg === 'number' || typeof arg === 'undefined') {
      return new Array(arg);
    } else {
      return arg;
    }
  }
}

var output = byteArray([
        0x54, 0x68, 0x65, 0x20, 0x77, 0x68, 0x6f, 0x6c,
        0x65, 0x20, 0x77, 0x6f, 0x72, 0x6c, 0x64, 0x20,
        0x69, 0x73, 0x20, 0x65, 0x6e, 0x64, 0x69, 0x6e,
        0x67, 0x2e, 0x0a
]);

var input = byteArray([
        0x04, 0x22, 0x4d, 0x18, 0x64, 0x40, 0xa7, 0x1b,
        0x00, 0x00, 0x80, 0x54, 0x68, 0x65, 0x20, 0x77,
        0x68, 0x6f, 0x6c, 0x65, 0x20, 0x77, 0x6f, 0x72,
        0x6c, 0x64, 0x20, 0x69, 0x73, 0x20, 0x65, 0x6e,
        0x64, 0x69, 0x6e, 0x67, 0x2e, 0x0a, 0x00, 0x00,
        0x00, 0x00, 0xbc, 0xa8, 0x6b, 0xc5
]);

console.log(lz4.decompress(input));
console.log(output);
 */
