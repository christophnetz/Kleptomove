#include <iostream>
#include <cine/cmd_line.h>
#include <cine/archive.hpp>


using namespace archive;


template <typename T, typename U>
void convert(const fs::path& tmp, const fs::path& arc, int G)
{
  iarch ia(arc);
  auto cm = ia.extract(G);
  auto dst = compressed_mem::buffer((unsigned char*)std::malloc(cm.un * cm.usize), std::free);
  uncompress(dst.get(), cm);
  size_t n = (cm.un * cm.usize) / sizeof(T);
  std::vector<U> res(n);
  const T* p = (const T*)dst.get();
  for (size_t i = 0; i < n; ++i) {
    res[i] = static_cast<U>(p[i]);
  }
  std::filebuf fb;
  fb.open(tmp, std::ios::out | std::ios::binary); 
  if (!fb.is_open()) throw std::runtime_error("can't create output file");
  fb.sputn((char*)res.data(), n * sizeof(U));
}


int main(int argc, const char** argv)
{
  try {
    cmd::cmd_line_parser clp(argc, argv);
    auto dir = clp.required<fs::path>("dir");
    if (clp.flag("--cleanup")) {
      fs::remove_all(dir / "tmp");
      return 0;
    }
    auto G = clp.required<int>("G");
    auto what = clp.required<std::string>("what");
    auto tmp = dir / "tmp";
    fs::create_directory(tmp);
    convert<float, double>(tmp / (what + "_fit.tmp"), dir / (what + "_fit.arc"), G);
    convert<int, int>(tmp / (what + "_anc.tmp"), dir / (what + "_anc.arc"), G);
    convert<float, double>(tmp / (what + "_ann.tmp"), dir / (what + "_ann.arc"), G);
    return 0;
  }
  catch (cmd::parse_error& err) {
    std::cerr << "\nParameter trouble: " << err.what() << '\n';
  }
  catch (std::exception& err) {
    std::cerr << "\nExeption caught: " << err.what() << '\n';
  }
  catch (...) {
    std::cerr << "\nUnknown exeption caught\n";
  }
  return 1;
}

