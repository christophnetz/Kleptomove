#ifndef ARCHIVE_HPP_INCLUDED
#define ARCHIVE_HPP_INCLUDED

#include <string>
#include <memory>
#include <vector>
#include <fstream>
#include <filesystem>


namespace fs = std::filesystem;


namespace archive {


  struct compressed_mem
  {
    using buffer = std::unique_ptr<unsigned char, decltype(std::free)*>;

    const uint32_t un;        // number of blobs
    const uint32_t usize;     // uncompressed blob-size [byte]
    const uint32_t csize;     // compressed buffer size
    buffer cbuf;              // compressed buffer
  };


  compressed_mem compress(const void* source, 
                          const size_t n, 
                          const size_t size, 
                          size_t stride = 0);


  void uncompress(void* dst, 
                  const compressed_mem& src, 
                  size_t stride = 0);


# pragma pack(push, 1)
  struct dict
  {
    const uint64_t ppos;      // position in stream
    const uint32_t csize;     // compressed size
    const uint32_t un;        // number of blobs
    const uint32_t usize;     // uncompressed blob-size [byte]
  };
# pragma pack(pop)


  class oarch
  {
  public:
    oarch() {}
    oarch(const fs::path& file, const std::string& header);
    ~oarch();

    void open(const fs::path& file, const std::string& header);
    void close();

    void insert(const compressed_mem& cm);

  private:
    std::vector<dict> dict_;
    std::filebuf fb_;
  };


  class iarch
  {
  public:
    explicit iarch(const fs::path& file);
    ~iarch();

    void open(const fs::path& file);
    void close();

    std::string header() const { return header_; }
    size_t size() const { return dict_.size(); }
    compressed_mem extract(size_t idx);

  private:
    std::vector<dict> dict_;
    std::string header_;
    std::filebuf fb_;
  };

}

#endif
