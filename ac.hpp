#include <bits/stdc++.h>
using namespace std;
class BitStream {
  public:
    BitStream(){ _pos = 0; _fpos=0; }
    ~BitStream(){}
    void push_back(bool bit){
        if(_pos%8==0){
            bitset<8> tmp;
            _data.push_back(tmp);
        }
        _data[_pos/8].set(_pos%8, bit);
        _pos++;
    }
    void set(int pos, bool bit){
        assert(pos < _pos);
        _data[pos/8].set(pos%8, bit);
    }
    bool get(int pos){
        assert(pos < _pos);
        return _data[pos/8][pos%8];
    }
    bool pop_front(){
        /* queue style pop, use only with ac
         */
        if (_fpos >= _pos) return 0;
        bool tmp = get(_fpos);
        _fpos++;
        return tmp;
    }
    bool pop_back(){
        /* queue style pop, use only with ans
         */
        if (_pos <= 0) return 0;
        _pos--;
        return _data[_pos/8][_pos%8];
    }
    int size(){ return _pos; }
  private:
    vector<bitset<8>> _data;
    int _pos;
    int _fpos;
};
class ArithmeticCodingEncoder {
  public:
    BitStream bit_stream;
    ArithmeticCodingEncoder(const int &precision){
        assert(precision >= 2 && precision <= std::numeric_limits<decltype(_full_range)>::digits);
        _precision = precision;
        _frequency_bits = std::numeric_limits<decltype(_full_range)>::digits - _precision;
        _frequency_bits = min(_frequency_bits, _precision - 2);
    	_full_range = static_cast<decltype(_full_range)>(1) << _precision;
        _half_range = _full_range >> 1;
        _quarter_range = _half_range >> 1;
        _three_forth_range = 3 * _quarter_range;
        _min_range = _quarter_range + 2;
        /* I actually have no idea why this min range limit is presented ... */
        _max_total = (static_cast<decltype(_full_range)>(1) << _frequency_bits) - 1;
        /* bits for max cdf: 
         *    should not greater than precision bits - 2
         *    should not greater than internal representation bits - precision bits */
        _mask = _full_range - 1;
        _low = 0;
        _high = _mask;
        _pending_bits=0;
    }
    ~ArithmeticCodingEncoder(){}
    void encode(const uint32_t &sym, const vector<uint32_t> &cdf, const uint32_t &cdf_bits){
        assert(_low < _high);
        assert((_low & _mask) == _low);
        assert((_high & _mask) == _high);
        uint64_t range = _high - _low + 1;
        assert(_min_range <= range);
        assert(range <= _full_range);
        uint64_t c_total = cdf[cdf.size() - 1];
        uint64_t c_low = cdf[sym];
        uint64_t c_high = cdf[sym + 1];
        assert(c_low != c_high);
        assert(c_total <= _max_total);
        assert((static_cast<decltype(c_total)>(1) << cdf_bits) == c_total);
        _high = _low + ((c_high * range) >> cdf_bits) - 1;
        /* -1 is a magic trick derived from
         * https://marknelson.us/posts/2014/10/19/data-compression-with-arithmetic-coding.html
         * honestly speaking I do not know why -1 is necessary */
        _low  = _low + ((c_low  * range) >> cdf_bits);
        while(1){
            if(_high<_half_range||_low>=_half_range){
                bool bit = static_cast<bool>(_high>=_half_range);
                bit_stream.push_back(bit);        
                for (; _pending_bits > 0; _pending_bits--)
                    bit_stream.push_back(!bit);
            }else if(_low>=_quarter_range&&_high<_three_forth_range){
                assert(_pending_bits < std::numeric_limits<decltype(_pending_bits)>::max());
                _pending_bits++;
                _low-=_quarter_range;
                _high-=_quarter_range;
            }else{
                break;
            }
            _high <<= 1;
            _high++;
            _low <<=1;
            _high &= _mask;
            _low &= _mask;
        }
    }
    void flush(){
        _pending_bits++;
        bool bit = static_cast<bool>(_low >= _quarter_range);
        bit_stream.push_back(bit);        
        for (; _pending_bits > 0; _pending_bits--)
            bit_stream.push_back(!bit);
    }
  private:
    uint64_t _precision;
    uint64_t _frequency_bits;
    uint64_t _full_range;
    uint64_t _half_range;
    uint64_t _quarter_range;
    uint64_t _three_forth_range;
    uint64_t _min_range;
    uint64_t _max_total;
    uint64_t _mask;
    uint64_t _low;
    uint64_t _high;
    uint64_t _pending_bits;
};
class ArithmeticCodingDecoder{
  public:
    BitStream bit_stream;
    ArithmeticCodingDecoder(const int &precision, const BitStream &encode_bit_stream){
        assert(precision >= 2 && precision <= std::numeric_limits<decltype(_full_range)>::digits);
        bit_stream = encode_bit_stream;
        _precision = precision;
        _frequency_bits = std::numeric_limits<decltype(_full_range)>::digits - _precision;
        _frequency_bits = min(_frequency_bits, _precision - 2);
    	_full_range = static_cast<decltype(_full_range)>(1) << _precision;
        _half_range = _full_range >> 1;
        _quarter_range = _half_range >> 1;
        _three_forth_range = 3 * _quarter_range;
        _min_range = _quarter_range + 2;
        _max_total = (static_cast<decltype(_full_range)>(1) << _frequency_bits) - 1;
        _mask = _full_range - 1;
        _low = 0;
        _high = _mask;
        _pending_bits = 0;
        _code = 0;
        for(int i=0;i<_precision;i++){
            _code <<= 1;
            _code += bit_stream.pop_front() ? 1: 0;
        }
    }
    ~ArithmeticCodingDecoder(){}
    uint32_t decode(const vector<uint32_t> &cdf, const uint32_t &cdf_bits){
        uint32_t c_total = cdf[cdf.size() - 1];
        assert((static_cast<decltype(c_total)>(1) << cdf_bits) == c_total);
        uint64_t range = _high - _low + 1;
        uint64_t scaled_range = _code - _low;
        uint64_t scaled_value = (((scaled_range + 1) << cdf_bits) - 1) / range;
        assert(scaled_value < c_total);
        uint64_t start = 0;
        uint64_t end = cdf.size()-1;
        while (end - start > 1) {
            uint64_t middle = (start + end) >> 1;
            if (cdf[middle] > scaled_value)
                end = middle;
            else
                start = middle;
        }
        assert(start + 1 == end);
        uint64_t sym = start;
        uint64_t c_low = cdf[sym];
        uint64_t c_high = cdf[sym + 1];
        assert(c_low != c_high);
        assert(c_total <= _max_total);
        _high = _low + ((c_high * range) >> cdf_bits) - 1;
        _low  = _low + ((c_low  * range) >> cdf_bits);
        while(1){
            if(_high < _half_range){
                // pass
            }else if(_low >= _half_range){
                _code -= _half_range;
                _low -= _half_range;
                _high -= _half_range;
            }else if(_low >= _quarter_range && _high < _three_forth_range){
                _code -= _quarter_range;
                _low -= _quarter_range;
                _high -= _quarter_range;
            }else{
                break;
            }
            _high <<= 1;
            _high++;
            _low <<=1;
            _code <<= 1;
            _code += bit_stream.pop_front() ? 1 : 0;
        }
        return static_cast<uint32_t>(sym);
    }
  private:
    uint64_t _precision;
    uint64_t _frequency_bits;
    uint64_t _full_range;
    uint64_t _half_range;
    uint64_t _quarter_range;
    uint64_t _three_forth_range;
    uint64_t _min_range;
    uint64_t _max_total;
    uint64_t _mask;
    uint64_t _low;
    uint64_t _high;
    uint64_t _pending_bits;
    uint64_t _code;
};
class RANSCodec {
  public:
    BitStream bit_stream;
    RANSCodec(const int &h_precision, const int &t_precision){
        _h_precision = h_precision;
        _t_precision = t_precision;
        assert(_t_precision < _h_precision);
        assert(_h_precision <= _t_precision * 2);
        _h_min = static_cast<decltype(_h_min)>(1) << (_h_precision - _t_precision);
        _state = _h_min; // max state
    }
    RANSCodec(const int &h_precision, const int &t_precision, const BitStream &encode_bit_stream){
        _h_precision = h_precision;
        _t_precision = t_precision;
        _h_min = static_cast<decltype(_h_min)>(1) << (_h_precision - _t_precision);
        bit_stream = encode_bit_stream;
         _state = 0;
        for(int i = _h_precision - 1; i >= 0; i--){
            uint64_t bit = bit_stream.pop_back();
            _state |= (bit << i);
        }
    }
    ~RANSCodec(){}
    void encode(const uint32_t &sym, const vector<uint32_t> &cdf, const uint32_t &cdf_bits){
        uint64_t c_low = cdf[sym];
        uint64_t c_range = cdf[sym + 1] - c_low;
        uint64_t c_total = cdf[cdf.size() - 1];
        assert((static_cast<decltype(c_total)>(1) << cdf_bits) == c_total);
        uint64_t state = _state;
        uint64_t state_max = c_range << (_h_precision - cdf_bits);
        // what does this 32 means ?
        if(state >= state_max){
            // encode 32 bits x into bitstream
            uint64_t mask = 1;
            for(int i = 1; i <= _t_precision; i++){
                bit_stream.push_back(static_cast<bool>(state & mask));
                mask <<= 1;
            }
            state >>= _t_precision;
            assert(state < state_max);
        }
        _state = ((state / c_range) << cdf_bits) + (state % c_range) + c_low;
    }
    void flush(){
        uint64_t mask = 1;
        for(int i = 1; i <= _h_precision; i++){
            bit_stream.push_back(static_cast<bool>(_state & mask));
            mask <<= 1;
        }
        _state = 0;
    }
    uint32_t decode(const vector<uint32_t> &cdf, const uint32_t &cdf_bits){
        uint64_t scaled_value = _state & ((static_cast<decltype(_state)>(1) << cdf_bits) - 1);
        uint64_t start = 0;
        uint64_t end = cdf.size()-1;
        while (end - start > 1) {
            uint64_t middle = (start + end) >> 1;
            if (cdf[middle] > scaled_value)
                end = middle;
            else
                start = middle;
        }
        assert(start + 1 == end);
        uint64_t sym = start;
        // printf("%u\n",static_cast<uint32_t>(sym));
        uint64_t c_low = cdf[sym];
        uint64_t c_range = cdf[sym + 1] - c_low;
        uint64_t state = _state;
        state = c_range * (state >> cdf_bits) + scaled_value - c_low;
        if (state < _h_min){
            state <<= _t_precision;
            for(int i = _t_precision - 1; i >= 0; i--){
                uint64_t bit = bit_stream.pop_back();
                state |= (bit << i);
            }
            assert (state >= _h_min);
        }
        _state = state;
        return static_cast<uint32_t>(sym);
    }
  private:
    uint64_t _state;
    uint64_t _h_precision;
    uint64_t _t_precision;
    uint64_t _t_mask;
    uint64_t _h_min;
};