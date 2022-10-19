
## YAECL: Yet Another Entropy Coding Library for Neural Compression Research
### Motivation
* yaecl is an Arithmetic Coding and Asymmetric Numeral Systems library supports C++ header only and Python. It is designed for researchers in neural compression (image, video, point cloud etc.) who wants to use Python to easily access different types of coding algorithms (fifo, filo) with decent efficiency. It is also for desirable industry researchers who need an unified library cross Python and C++.

|              | Platform           | Coding Algo          | Coding Mode | Cdf Mode     | Cdf Precision Ctrl | Codec Precision Ctrl |
|--------------|--------------------|----------------------|-------------|--------------|--------------------|----------------------|
| yaecl        | C++ header, Python | AC, RANS             | 1x1,1xn,nxn | int (any w)  | Yes                | Yes                  |
| constriction | Rust, Python       | Huffman, RANGE, RANS | 1x1,1xn,nxn | float        | No                 | No                   |
| torchac      | Python (torch)     | AC                   | nxn         | float, int16 | No                 | No                   |
| nayuki       | C++, Python, Java  | AC                   | 1x1         | uint32       | Yes                | Yes                  |
| ryg_rans     | C header           | RANS                 | 1x1         | uint32       | Yes                | No                   |
| Turbo        | C/C++              | RANGE, AC            | 1x1         | int (many w) | Yes                | Yes                  |
* references:
  * constriction: https://bamler-lab.github.io/constriction/
  * torchac: https://github.com/fab-jul/torchac
  * nayuki: https://github.com/nayuki/Reference-arithmetic-coding
  * ryg_rans: https://github.com/rygorous/ryg_rans
  * turbo: https://github.com/powturbo/Turbo-Range-Coder
* with a unified interface, yaecl support both:
  * queue style fifo coding for auto-regressive coding
  * stack stype filo coding for bits-back coding
* furthermore, with an unified interface, yaecl support both:
  * python usage for research
  * C++ header only usage for practical deployment as SDK with reasonable efficiency.
* furthermore, with an unified interface, yaecl support:
  * flexible templated style tailor of any precision / speed trade-off (your compiler support)
  * bit level precision control
* furthermore, with an unified interface using python "memoryview" and python "buffer protocal", yaecl python support:
  * any framework / lib supporting memoryview, including python native list, numpy
  * with highly efficient io, as basically only pointers are passed between python and c++ backend
  * can be efficiently and easily used with Pytorch and Tensorflow with numpy intermediate
* furthermore, with an unified interface, yaecl python support:
  * 1 symbol 1 cdf coding, as the BB-ANS paper
  * n symbol 1 cdf coding, as the p(z) part in google-hyperprior
  * n symbol n cdf coding, as the p(y|z) part in google-hyperprior
### Install: C++ Single Include
* put yaecl.hpp to your include path, then just:
  ```cpp
  #include "yaecl.hpp"
  using namespace yaecl;
  ```
### Usage: C++
* prepare your symbol, note that the symbol start with 0, so an alphabet of size n contains {0, ..., n-1}
* prepare your cdf, note that for alphabet of size n, your cdf should have n+1 bin, with last bin == 2 ^ cdf_bits
* e.g. alphabet = {0, 1, 2, 3, 4}, p(i) = 0.2, cdf_bits = 16
  ```cpp
  uint32_t cdf_max= (1 << 16);
  uint32_t cdf[6] = {0, 0.2 * cdf_max, 0.4 * cdf_max, 0.6 * cdf_max, 0.8 * cdf_max, cdf_max};
  ```
* to encode, call encode method of encoder:
* note that the first argument is symbol, the second is ptr to cdf, the third is cdf bits, or log2(cdf_max_value)
  ```cpp
  ArithmeticCodingEncoder<uint64_t, uint32_t> ace=ArithmeticCodingEncoder<uint64_t, uint32_t>(32);
  for(int i=1;i<=test_n;i++) ace.encode(i % 5, cdf, 16);
  ace.flush();
  ace.bit_stream.save("path"); // optionally
  ```
* you need to call flush after encoding complete, and optionally save bit stream
* to init a decoder, you need a bit stream ,either from encoder / file
  ```cpp
  BitStream bs = ace.bit_stream;
  // BitStream bs; bs.load("path"); 
  ArithmeticCodingDecoder<uint64_t, uint32_t> acd = ArithmeticCodingDecoder<uint64_t, uint32_t>(32, bs);
  for(int i=1;i<=test_n;i++) assert(static_cast<int>(acd.decode(5, cdf, 16)) == i%5);
  ```
* note that 5 is the number of alphabet. You have to provide it during decoding
* refer to yaecl_test.cpp for more examples, and yaecl.hpp for more docs

### Install: Python
* build with CMake and pybind11:
  ```bash
  mkdir build
  cd build
  cmake ../ -DPYTHON_EXECUTABLE:FILEPATH=$YOUR_PYTHON_BIN -DPYTHON_INCLUDE_DIR:PATH=$YOUR_PYTHON_INCLUDE -DPYTHON_LIBRARY:FILEPATH=$YOUR_PYTHON_LIB
  make -j
  ```
* example of cmake command with conda:
  ```bash
  cmake ../ -DPYTHON_EXECUTABLE:FILEPATH=/home/xx/.conda/envs/yy/bin/python -DPYTHON_INCLUDE_DIR:PATH=/home/xx/.conda/envs/yy/include/python3.10 -DPYTHON_LIBRARY:FILEPATH=/home/xx/.conda/envs/yy/lib/libpython3.10.so
  ```
* this produce a library yaecl.cpython-version-platform.so, move it to your python path, then directly use it in python:
  ```python
  import yaecl
  ```
### Usage: Python
* we provide extra wrapper for python other than en/de code symbol by symbol. You can choose to encode per
symbol per pdf, n symbol per cdf or n symbol n cdf. Below the 3 ways are equivalent:
  ```python
  cnt = 32*48*320
  cdf_max = 2 ** 16
  cdf = np.array([0, 0.2 * cdf_max, 0.4 * cdf_max, 0.6 * cdf_max, 0.8 * cdf_max, cdf_max], dtype=np.int32)
  
  # encode symbol by symbol
  ac_enc = yaecl.ac_encoder_t()
  for i in range(cnt):
      ac_enc.encode(i % 5, memoryview(cdf), 16)
  ac_enc.flush()
  
  # encode n x 1
  sym_b = np.array([i % 5 for i in range(cnt)], dtype=np.int32)
  symd_b = np.array([0 for _ in range(cnt)], dtype=np.int32)
  ac_enc = yaecl.ac_encoder_t()
  ac_enc.encode_nx1(sym_b, memoryview(cdf), 16)
  ac_enc.flush()
  
  # encode n x n
  sym_b = np.array([i % 5 for i in range(cnt)], dtype=np.int32)
  cdf_b = np.array([cdf for _ in range(cnt)], dtype=np.int32)
  ac_enc = yaecl.ac_encoder_t()
  ac_enc.encode_nxn(sym_b, cdf_b, 16)
  ac_enc.flush()
  ```
  * same thing happens to decoder. It is important to note that 1x1 decode return symbol, but nx1 decode and nxn decode
  require user to pass a buffer to hold the decoded symbols
  ```python
  # decode 1 x 1
  ac_dec = yaecl.ac_decoder_t(ac_enc.bit_stream)
  for i in range(cnt):
      de_sym = ac_dec.decode(5, memoryview(cdf), 16)

  # decode n x n
  symd_b = np.array([0 for _ in range(cnt)], dtype=np.int32)
  ac_dec = yaecl.ac_decoder_t(ac_enc.bit_stream)
  ac_dec.decode_nxn(5, memoryview(cdf_b), 16, memoryview(symd_b))
  ```
* those convenient wrappers avoids loop in python, which is efficient
* another point to make is that all the array-like data are passed by memoryview, is sort of like pass by pointer.
  * Read more on memoryview: https://stackoverflow.com/questions/18655648/what-exactly-is-the-point-of-memoryview-in-python
  * Read more on memoryview: https://docs.python.org/3/c-api/memoryview.html
  * Read more on memoryview: https://pybind11.readthedocs.io/en/stable/reference.html
* See more examples in yaecl_test.py and yaecl.hpp and yaecl_python.cpp

## Reference
* if you use this lib in your paper, cite our paper:
  ```
  @article{xu2022bit,
  title={Bit allocation using optimization},
  author={Xu, Tongda and Gao, Han and Gao, Chenjian and Pi, Jinyong and Li, Yanghao and Wang, Yuanyuan and Zhu, Ziyu and He, Dailan and Ye, Mao and Qin, Hongwei and others},
  journal={arXiv preprint arXiv:2209.09422},
  year={2022}
  }
  ```