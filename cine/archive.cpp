#include "archive.hpp"
#include <stdexcept>
#include <zlib/zlib.h>


namespace archive {


  compressed_mem compress(const void* source, const size_t n, const size_t size, size_t stride)
  {
    const uLong bytes = static_cast<uLong>(n * size);
    uLong destLen = static_cast<uLong>(1.1 * bytes + 128);
    auto dst = compressed_mem::buffer((unsigned char*)std::malloc(destLen), std::free);
    stride = stride ? stride : size;
    const uLong sourceLen = bytes;
    int Z_RES;
    if (stride == size) {
      Z_RES = ::compress(dst.get(), &destLen, (const Bytef*)source, sourceLen);
    }
    else {
      std::vector<unsigned char> ibuf(bytes);
      for (size_t i = 0; i < n; ++i) {
        std::memcpy(ibuf.data() + i * size, (const char*)source + i * stride, size);
      }
      Z_RES = ::compress(dst.get(), &destLen, (const Bytef*)ibuf.data(), sourceLen);
    }
    if (Z_OK != Z_RES) {
      throw std::runtime_error("compression failed");
    }
    return {static_cast<uint32_t>(n), static_cast<uint32_t>(size), destLen, std::move(dst)};
  }


  void uncompress(void* dst, const compressed_mem& src, size_t stride)
  {
    uLong destLen = static_cast<uLong>(src.un * src.usize + 128);
    std::vector<unsigned char> ubuf(destLen);
    if (Z_OK != ::uncompress(ubuf.data(), &destLen, (const Bytef*)src.cbuf.get(), src.csize)) {
      throw std::runtime_error("decmpression failed");
    }
    if (stride == 0 || src.usize == stride) {
      std::memcpy(dst, ubuf.data(), static_cast<uLong>(src.un) * src.usize);
    }
    else {
      for (size_t i = 0; i < src.un; ++i) {
        std::memcpy((char*)dst + i * stride, ubuf.data() + i * src.usize, src.usize);
      }
    }
  }


  oarch::oarch(const fs::path& file, const std::string& header)
  {
    open(file, header);
  }


  oarch::~oarch()
  {
    close();
  }


  void oarch::open(const fs::path& file, const std::string& header)
  {
    if (header.empty() || header.size() > 255) throw std::runtime_error("oarch: invalid header");
    if (fb_.is_open()) close();
    fb_.open(file, std::ios::out | std::ios::binary); 
    if (!fb_.is_open()) throw std::runtime_error("can't create oarch");

    // insert magic number for endianness test
    int32_t magic = 0x49484148;   // little endian ascii: 'HAHI'
    fb_.sputn((char*)&magic, 4);

    // insert placeholder for dictionary offset
    uint64_t dictofs(0);
    fb_.sputn((char*)&dictofs, 8);

    // write header
    unsigned char header_size = static_cast<unsigned char>(header.size());
    fb_.sputc(header_size);
    fb_.sputn(header.data(), header_size);
    dict_.clear();
  }


  void oarch::close()
  {
    if (!fb_.is_open()) return;
    // write dictionary
    uint64_t pdict = fb_.pubseekoff(0, std::ios_base::end);
    uint32_t dsize = static_cast<uint32_t>(dict_.size());
    fb_.sputn((char*)&dsize, 4);
    if (dsize) fb_.sputn((char*)dict_.data(), dsize * sizeof(dict));    
    
    // write position of dictionary
    fb_.pubseekoff(4, std::ios_base::beg);
    fb_.sputn((char*)&pdict, 8);
    dict_.clear();
    fb_.close();
  }


   void oarch::insert(const compressed_mem& cm)
   {
     uint64_t pend = fb_.pubseekoff(0, std::ios_base::end);
     fb_.sputn((char*)cm.cbuf.get(), cm.csize);
     dict_.push_back({pend, cm.csize, cm.un, cm.usize});
   }


   iarch::iarch(const fs::path & file)
   {
     open(file);
   }

   
   iarch::~iarch()
   {
     close();
   }


   void iarch::open(const fs::path & file)
   {
     if (fb_.is_open()) close();
     fb_.open(file, std::ios::in | std::ios::binary); 
     if (!fb_.is_open()) throw std::runtime_error("can't open iarch");

     // read magic number for endianess test
     int32_t magic = 0;
     fb_.sgetn((char*)&magic, 4);
     if (magic == 0x41484948) throw std::runtime_error("oarch: invalid endianness");
     if (magic != 0x49484148) throw std::runtime_error("oarch: corrupt archive");

     // read dictionary offset
     uint64_t pdict(0);
     fb_.sgetn((char*)&pdict, 8);

     // read header
     int header_size = fb_.sbumpc();
     header_.resize(header_size);
     fb_.sgetn((char*)header_.data(), header_size);

     // read dictionary
     fb_.pubseekoff(pdict, std::ios_base::beg);
     uint32_t dsize = static_cast<uint32_t>(dict_.size());
     fb_.sgetn((char*)&dsize, 4);
     dict_.resize(dsize, {0,0,0,0});
     fb_.sgetn((char*)dict_.data(), dsize * sizeof(dict));
   }


   void iarch::close()
   {
     dict_.clear();
     fb_.close();
   }


   compressed_mem iarch::extract(size_t idx)
   {
     if (idx >= dict_.size()) throw std::runtime_error("oarchive: invalid index");
     const auto& dict = dict_[idx];
     fb_.pubseekoff(dict.ppos, std::ios_base::beg);

     auto cbuf = compressed_mem::buffer((unsigned char*)std::malloc(dict.csize), std::free);
     fb_.sgetn((char*)cbuf.get(), dict.csize);
     return {dict.un, dict.usize, dict.csize, std::move(cbuf)};
   }

}
