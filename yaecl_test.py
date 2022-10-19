import numpy as np
import yaecl
from timeit import default_timer as timer

cnt = 32*48*320
cdf_max = 2 ** 16
cdf = np.array([0, 0.2 * cdf_max, 0.4 * cdf_max, 0.6 * cdf_max, 0.8 * cdf_max, cdf_max], dtype=np.int32)

def test_ac_1x1():
    ac_enc = yaecl.ac_encoder_t()
    start = timer()
    for i in range(cnt):
        ac_enc.encode(i % 5, memoryview(cdf), 16)
    ac_enc.flush()
    end = timer()
    print("ac naive encoding elapse: {0:.4f} s".format(end - start))
    ac_dec = yaecl.ac_decoder_t(ac_enc.bit_stream)
    start = timer()
    for i in range(cnt):
        de_sym = ac_dec.decode(5, memoryview(cdf), 16)
        assert(de_sym == i % 5)
    end = timer()
    print("ac naive decoding elapse: {0:.4f} s".format(end - start))

def test_ac_nx1():
    sym_b = np.array([i % 5 for i in range(cnt)], dtype=np.int32)
    symd_b = np.array([0 for _ in range(cnt)], dtype=np.int32)
    ac_enc = yaecl.ac_encoder_t()
    ac_enc.encode_nx1(sym_b, memoryview(cdf), 16)
    ac_enc.flush()
    ac_dec = yaecl.ac_decoder_t(ac_enc.bit_stream)
    ac_dec.decode_nx1(5, memoryview(cdf), 16, symd_b)
    assert(np.sum(np.abs(sym_b - symd_b)) == 0)

def test_ac_nxn():
    sym_b = np.array([i % 5 for i in range(cnt)], dtype=np.int32)
    cdf_b = np.array([cdf for _ in range(cnt)], dtype=np.int32)
    symd_b = np.array([0 for _ in range(cnt)], dtype=np.int32)
    ac_enc = yaecl.ac_encoder_t()
    start = timer()
    ac_enc.encode_nxn(sym_b, cdf_b, 16)
    ac_enc.flush()
    end = timer()
    print("ac batch encoding elapse: {0:.4f} s".format(end - start))
    ac_dec = yaecl.ac_decoder_t(ac_enc.bit_stream)
    start = timer()
    ac_dec.decode_nxn(5, memoryview(cdf_b), 16, memoryview(symd_b))
    end = timer()
    print("ac batch decoding elapse: {0:.4f} s".format(end - start))
    assert(np.sum(np.abs(sym_b - symd_b)) == 0)

def test_rans_1x1():
    rans_enc = yaecl.rans_codec_t()
    start = timer()
    for i in range(cnt):
        rans_enc.encode(i % 5, memoryview(cdf), 16)
    rans_enc.flush()
    end = timer()
    print("rans naive encoding elapse: {0:.4f} s".format(end - start))
    rans_dec = yaecl.rans_codec_t(rans_enc.bit_stream)
    start = timer()
    for i in reversed(range(cnt)):
        de_sym = rans_dec.decode(5, memoryview(cdf), 16)
        assert(de_sym == i % 5)
    end = timer()
    print("rans naive decoding elapse: {0:.4f} s".format(end - start))

def test_rans_1x1_interactive():
    rans_codec = yaecl.rans_codec_t()
    start = timer()
    for i in range(cnt):
        rans_codec.encode(i % 5, memoryview(cdf), 16)
    end = timer()
    print("rans naive encoding elapse: {0:.4f} s".format(end - start))
    start = timer()
    for i in reversed(range(cnt)):
        de_sym = rans_codec.decode(5, memoryview(cdf), 16)
        assert(de_sym == i % 5)
    end = timer()
    print("rans naive decoding elapse: {0:.4f} s".format(end - start))

test_ac_1x1()
test_ac_nx1()
test_ac_nxn()
test_rans_1x1()
test_rans_1x1_interactive()