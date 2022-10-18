#include "ac.hpp"
int main(){
    int test_n = 1024;
    uint32_t cdf_max= (1 << 16);
    vector<uint32_t> cdf = {0, 0.2 * cdf_max, 0.4 * cdf_max, 0.6 * cdf_max, 0.8 * cdf_max, cdf_max};
    printf("[test] testing arithmetic coding\n");
    ArithmeticCodingEncoder ace=ArithmeticCodingEncoder(32);
    for(int i=1;i<=test_n;i++){
        ace.encode(i % 5, cdf, 16);
    }
    ace.flush();
    printf("[test] -- actual size: %d --- ideal info: %.2f\n", ace.bit_stream.size(), test_n * 2.3219);
    ArithmeticCodingDecoder acd = ArithmeticCodingDecoder(32, ace.bit_stream);
    for(int i=1;i<=test_n;i++){
        assert(static_cast<int>(acd.decode(cdf, 16)) == i%5);
    }
    printf("[test] -- decode success\n");
    printf("[test] testing rans interactive coding\n");
    RANSCodec ranscd = RANSCodec(64, 32);
    for(int i=1;i<=test_n;i++){
        ranscd.encode(i % 5, cdf, 16);
    }
    printf("[test] -- actual size: %d --- ideal info: %.2f\n", ranscd.bit_stream.size(), test_n * 2.3219);
    for(int i=test_n;i>=1;i--){
        assert(static_cast<int>(ranscd.decode(cdf, 16)) == i%5);
    }
    printf("[test] -- decode success\n");
    printf("[test] testing rans separate coding\n");
    RANSCodec ranse = RANSCodec(64, 32);
    for(int i=1;i<=test_n;i++){
        ranse.encode(i % 5, cdf, 16);
    }
    ranse.flush();
    printf("[test] -- actual size: %d --- ideal info: %.2f\n", ranse.bit_stream.size(), test_n * 2.3219);
    RANSCodec ransd = RANSCodec(64, 32, ranse.bit_stream);
    for(int i=test_n;i>=1;i--){
        assert(static_cast<int>(ransd.decode(cdf, 16)) == i%5);
    }
}