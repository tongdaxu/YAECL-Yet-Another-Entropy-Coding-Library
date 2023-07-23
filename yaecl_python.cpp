#include <pybind11/pybind11.h>
#include "yaecl.hpp"

using namespace yaecl;
using namespace pybind11;

template <typename T_in, typename T_out>
/* template args: see ArithmeticCodingEncoder */
class PYArithmeticCodingEncoder : public ArithmeticCodingEncoder<T_in, T_out> {
  public:
    PYArithmeticCodingEncoder(): ArithmeticCodingEncoder<T_in, T_out>(32) {}
    PYArithmeticCodingEncoder(const int &precision): ArithmeticCodingEncoder<T_in, T_out>(precision) {
        /* args: See ArithmeticCodingEncoder */
    }
    ~PYArithmeticCodingEncoder(){}
    void encode(const int &sym, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingEncoder 
         * cdf_buf:
         * * 1D memoryview of cdf array
         * * * dim 1 = alphabet size + 1
         * * e.g.: memoryview(np.array([...]))
         * * see more: https://pybind11.readthedocs.io/en/stable/reference.html
         */
        buffer_info cdf_info = cdf_buf.request();
        ArithmeticCodingEncoder<T_in, T_out>::encode(sym,
                                                     reinterpret_cast<T_out*> (cdf_info.ptr),
                                                     cdf_bits);
    }
    void encode_nx1(const buffer &sym_buf, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingEncoder 
         * sym_buf
         * * 1D memory view of symbol array
         * * * dim 1 = N (symbol to encode)
         * cdf_buf:
         * * 1D memoryview of cdf array
         * * * dim 1 = alphabet size + 1
         */
        buffer_info sym_info = sym_buf.request();
        buffer_info cdf_info = cdf_buf.request();
        assert(static_cast<int>(sym_info.ndim) == 1 && static_cast<int>(cdf_info.ndim) == 1);
        for(int i = 0; i < sym_info.shape[0]; i++){
            ArithmeticCodingEncoder<T_in, T_out>::encode((reinterpret_cast<T_out*>(sym_info.ptr))[i],
                                                          reinterpret_cast<T_out*>(cdf_info.ptr),
                                                          cdf_bits);
        }
    }
    void encode_nxn(const buffer &sym_buf, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingEncoder 
         * sym_buf
         * * 1D memory view of symbol array
         * * * dim 1 = N (symbol to encode)
         * cdf_buf:
         * * 2D memoryview of cdf array
         * * * dim 1 = N (symbol to encode)
         * * * dim 2 = alphabet size + 1
         */
        buffer_info sym_info = sym_buf.request();
        buffer_info cdf_info = cdf_buf.request();
        assert(static_cast<int>(sym_info.ndim == 1) && static_cast<int>(cdf_info.ndim) == 2);
        assert(static_cast<int>(sym_info.shape[0]) == static_cast<int>(cdf_info.shape[0]));
        assert(sizeof(T_out) == cdf_info.strides[1]);
        for(int i = 0; i < sym_info.shape[0]; i++){
            ArithmeticCodingEncoder<T_in, T_out>::encode((reinterpret_cast<T_out*>(sym_info.ptr))[i],
                                                          reinterpret_cast<T_out*>(cdf_info.ptr) + i * (cdf_info.strides[0] / cdf_info.strides[1]),
                                                          cdf_bits);
        }
    }
};
template <typename T_in, typename T_out>
/* template args: see ArithmeticCodingEncoder */
class PYArithmeticCodingDecoder : public ArithmeticCodingDecoder<T_in, T_out> {
  public:
    PYArithmeticCodingDecoder(const BitStream &encode_bit_stream): ArithmeticCodingDecoder<T_in, T_out>(32, encode_bit_stream) {
        /* args: See ArithmeticCodingDecoder */
    }
    PYArithmeticCodingDecoder(const int &precision, const BitStream &encode_bit_stream): ArithmeticCodingDecoder<T_in, T_out>(precision, encode_bit_stream) {
        /* args: See ArithmeticCodingDecoder */
    }
    ~PYArithmeticCodingDecoder(){}
    T_out decode(const int &sym_cnt, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingDecoder */
        buffer_info cdf_info = cdf_buf.request();
        return ArithmeticCodingDecoder<T_in, T_out>::decode(sym_cnt,
                                                            reinterpret_cast<T_out*> (cdf_info.ptr),
                                                            cdf_bits);
    }
    void decode_nx1(const int &sym_cnt, const buffer &cdf_buf, const int &cdf_bits, const buffer &out_buf){
        /* args: See ArithmeticCodingDecoder, PYArithmeticCodingEncoder
         * out_buf
         * * 1D memory view of empty array to hold decoded symbols 
         * * * dim 1 = N (symbol to decode)
         */
        buffer_info cdf_info = cdf_buf.request();
        buffer_info out_info = out_buf.request();
        assert(static_cast<int>(out_info.ndim) == 1 && static_cast<int>(cdf_info.ndim) == 1);
        for(int i = 0; i < out_info.shape[0]; i++){
            reinterpret_cast<T_out*>(out_info.ptr)[i] = ArithmeticCodingDecoder<T_in, T_out>::decode(sym_cnt,
                                                                                                     reinterpret_cast<T_out*> (cdf_info.ptr),
                                                                                                     cdf_bits);

        }
    }
    void decode_nxn(const int &sym_cnt, const buffer &cdf_buf, const int &cdf_bits, const buffer &out_buf){
        /* args: See ArithmeticCodingDecoder, PYArithmeticCodingEncoder, decode_nx1 */
        buffer_info cdf_info = cdf_buf.request();
        buffer_info out_info = out_buf.request();
        assert(static_cast<int>(out_info.ndim) == 1 && static_cast<int>(cdf_info.ndim) == 2);
        assert(static_cast<int>(out_info.shape[0]) == static_cast<int>(cdf_info.shape[0]));
        for(int i = 0; i < out_info.shape[0]; i++){
            reinterpret_cast<T_out*>(out_info.ptr)[i] = ArithmeticCodingDecoder<T_in, T_out>::decode(sym_cnt,
                                                                                                     reinterpret_cast<T_out*> (cdf_info.ptr) + i * (cdf_info.strides[0] / cdf_info.strides[1]),
                                                                                                     cdf_bits);

        }
    }
};
template <typename T_in, typename T_out>
/* template args: see ArithmeticCodingEncoder */
class PYRANSCodec : public RANSCodec<T_in, T_out> {
  public:
    PYRANSCodec(): RANSCodec<T_in, T_out>(64, 32) {}
    PYRANSCodec(const int &h_precision, const int &t_precision): RANSCodec<T_in, T_out>(h_precision, t_precision) {
        /* args: See RANSCodec */
    }
    PYRANSCodec(const BitStream &encode_bit_stream): RANSCodec<T_in, T_out>(64, 32, encode_bit_stream) {
        /* args: See RANSCodec */
    }
    PYRANSCodec(const int &h_precision, const int &t_precision, const BitStream &encode_bit_stream): RANSCodec<T_in, T_out>(h_precision, t_precision) {
        /* args: See RANSCodec */
    }
    ~PYRANSCodec(){}
    void encode(const int &sym, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingEncoder */
        buffer_info cdf_info = cdf_buf.request();
        RANSCodec<T_in, T_out>::encode(sym,
                                       reinterpret_cast<T_out*> (cdf_info.ptr),
                                       cdf_bits);
    }
    void encode_nx1(const buffer &sym_buf, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingEncoder, PYArithmeticCodingEncoder */
        buffer_info sym_info = sym_buf.request();
        buffer_info cdf_info = cdf_buf.request();
        assert(static_cast<int>(sym_info.ndim) == 1 && static_cast<int>(cdf_info.ndim) == 1);
        for(int i = 0; i < sym_info.shape[0]; i++){
            RANSCodec<T_in, T_out>::encode((reinterpret_cast<T_out*>(sym_info.ptr))[i],
                                            reinterpret_cast<T_out*>(cdf_info.ptr),
                                            cdf_bits);
        }
    }
    void encode_nxn(const buffer &sym_buf, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingEncoder, PYArithmeticCodingEncoder */
        buffer_info sym_info = sym_buf.request();
        buffer_info cdf_info = cdf_buf.request();
        assert(static_cast<int>(sym_info.ndim == 1) && static_cast<int>(cdf_info.ndim) == 2);
        assert(static_cast<int>(sym_info.shape[0]) == static_cast<int>(cdf_info.shape[0]));
        assert(sizeof(T_out) == cdf_info.strides[1]);
        for(int i = 0; i < sym_info.shape[0]; i++){
            RANSCodec<T_in, T_out>::encode((reinterpret_cast<T_out*>(sym_info.ptr))[i],
                                            reinterpret_cast<T_out*>(cdf_info.ptr) + i * (cdf_info.strides[0] / cdf_info.strides[1]),
                                            cdf_bits);
        }
    }
    T_out decode(const int &sym_cnt, const buffer &cdf_buf, const int &cdf_bits){
        /* args: See ArithmeticCodingDecoder, PYArithmeticCodingDecoder */
        buffer_info cdf_info = cdf_buf.request();
        return RANSCodec<T_in, T_out>::decode(sym_cnt,
                                              reinterpret_cast<T_out*> (cdf_info.ptr),
                                              cdf_bits);
    }
    void decode_nx1(const int &sym_cnt, const buffer &cdf_buf, const int &cdf_bits, const buffer &out_buf){
        /* args: See ArithmeticCodingDecoder, PYArithmeticCodingDecoder */
        buffer_info cdf_info = cdf_buf.request();
        buffer_info out_info = out_buf.request();
        assert(static_cast<int>(out_info.ndim) == 1 && static_cast<int>(cdf_info.ndim) == 1);
        for(int i = 0; i < out_info.shape[0]; i++){
            reinterpret_cast<T_out*>(out_info.ptr)[i] = RANSCodec<T_in, T_out>::decode(sym_cnt,
                                                                                       reinterpret_cast<T_out*> (cdf_info.ptr),
                                                                                       cdf_bits);
        }
    }
    void decode_nxn(const int &sym_cnt, const buffer &cdf_buf, const int &cdf_bits, const buffer &out_buf){
        /* args: See ArithmeticCodingDecoder, PYArithmeticCodingDecoder */
        buffer_info cdf_info = cdf_buf.request();
        buffer_info out_info = out_buf.request();
        assert(static_cast<int>(out_info.ndim) == 1 && static_cast<int>(cdf_info.ndim) == 2);
        assert(static_cast<int>(out_info.shape[0]) == static_cast<int>(cdf_info.shape[0]));
        for(int i = 0; i < out_info.shape[0]; i++){
            reinterpret_cast<T_out*>(out_info.ptr)[i] = RANSCodec<T_in, T_out>::decode(sym_cnt,
                                                                                       reinterpret_cast<T_out*> (cdf_info.ptr) + i * (cdf_info.strides[0] / cdf_info.strides[1]),
                                                                                       cdf_bits);
        }
    }
};
typedef BitStream bit_stream_t;
typedef PYArithmeticCodingEncoder<uint64_t, int> ac_encoder_t;
typedef PYArithmeticCodingDecoder<uint64_t, int> ac_decoder_t;
typedef PYRANSCodec<uint64_t, int> rans_codec_t;
/* you can define your own type with any width and add it to PYBIND11_MODULE
 * see more: https://pybind11.readthedocs.io/en/stable/
 */
PYBIND11_MODULE(yaecl, m) {
    m.doc() = "yaecl python library";
    class_<bit_stream_t>(m, "bit_stream_t")
        .def(init<>())
        .def("size", &bit_stream_t::size)
        .def("save", &bit_stream_t::save)
        .def("load", &bit_stream_t::load)
        .def_property("data", &bit_stream_t::getData, &bit_stream_t::setData, "py::bytes");
    class_<ac_encoder_t>(m, "ac_encoder_t")
        .def(init<>())
        .def(init<const int &>())
        .def_readwrite("bit_stream", &ac_encoder_t::bit_stream)
        .def("encode", &ac_encoder_t::encode)
        .def("encode_nx1", &ac_encoder_t::encode_nx1)
        .def("encode_nxn", &ac_encoder_t::encode_nxn)
        .def("flush", &ac_encoder_t::flush);
    class_<ac_decoder_t>(m, "ac_decoder_t")
        .def(init<const bit_stream_t &>())
        .def(init<const int &, const bit_stream_t &>())
        .def_readwrite("bit_stream", &ac_decoder_t::bit_stream)
        .def("decode", &ac_decoder_t::decode)
        .def("decode_nx1", &ac_decoder_t::decode_nx1)
        .def("decode_nxn", &ac_decoder_t::decode_nxn);
    class_<rans_codec_t>(m, "rans_codec_t")
        .def(init<>())
        .def(init<const int &, const int &>())
        .def(init<const bit_stream_t &>())
        .def(init<const int &, const int &, const bit_stream_t &>())
        .def_readwrite("bit_stream", &rans_codec_t::bit_stream)
        .def("encode", &rans_codec_t::encode)
        .def("encode_nx1", &rans_codec_t::encode_nx1)
        .def("encode_nxn", &rans_codec_t::encode_nxn)
        .def("flush", &rans_codec_t::flush)
        .def("decode", &rans_codec_t::decode)
        .def("decode_nx1", &rans_codec_t::decode_nx1)
        .def("decode_nxn", &rans_codec_t::decode_nxn);
}
