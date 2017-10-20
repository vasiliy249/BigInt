#include "stdafx.h"

namespace bio = boost::iostreams;
namespace po = boost::program_options;
namespace fs = boost::filesystem;

typedef bio::basic_mapped_file_params<fs::path> path_mapped_file_params;
typedef std::vector<std::pair<uintmax_t, uintmax_t> > ChunkRanges;

const uintmax_t kBytesInMb = 1024 * 1024;
const int kMinInt = std::numeric_limits<int>::min();
const uintmax_t kDefaultOutputFileSizeMb = 2000;

void thread_func(const char* data, uintmax_t size, int* max) {
  const int* start = reinterpret_cast<const int*>(data);
  const int* cursor = reinterpret_cast<const int*>(data);
  while (cursor < start + size / 4) {
    if (*cursor > *max) {
      *max = *cursor;
    }
    ++cursor;
  }
}

int boost_func(const std::string& path_to_data,
                int thread_count,
                uintmax_t offset,
                uintmax_t length)
{
  path_mapped_file_params map_params;
  map_params.path = path_to_data;
  map_params.offset = offset;
  map_params.flags = bio::mapped_file_base::readonly;
  map_params.length = static_cast<size_t>(length);
  bio::mapped_file_source file_map;
  file_map.open(map_params);
  const char* data_ptr = file_map.data();
  uintmax_t total_size = (file_map.size() / 4) * 4;
  uintmax_t thr_chunk_size = total_size / thread_count;
  uintmax_t tail_size = total_size - thr_chunk_size * thread_count;

  std::vector<std::thread> threads(thread_count);
  std::vector<int> maxs(thread_count, kMinInt);
  for (int i = 0; i < thread_count - 1; ++i) {
    threads[i] = std::thread(thread_func, data_ptr + i * thr_chunk_size, thr_chunk_size, &maxs[0]);
  }
  thread_func(data_ptr + (thread_count - 1) * thr_chunk_size,
              thr_chunk_size + tail_size, &maxs[thread_count - 1]);
  for (int i = 0; i < thread_count - 1; ++i) {
    threads[i].join();
  }
  return *std::max_element(maxs.begin(), maxs.end());
}

void winapi_func(const std::string& path_to_data, int thread_count) {
  HANDLE hFile = CreateFile(path_to_data.c_str(), GENERIC_READ,
                            0, nullptr, OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
  if (hFile == INVALID_HANDLE_VALUE) {
    std::cerr << "fileMappingCreate - CreateFile failed, fname =  "
      << path_to_data << std::endl;
    return;
  }

  DWORD dwFileSize = GetFileSize(hFile, nullptr);
  if (dwFileSize == INVALID_FILE_SIZE) {
    std::cerr << "fileMappingCreate - GetFileSize failed, fname =  "
      << path_to_data << std::endl;
    CloseHandle(hFile);
    return;
  }

  HANDLE hMapping = CreateFileMapping(hFile, nullptr, PAGE_READONLY, 0, 0,
                                      nullptr);
  if (hMapping == nullptr) {
    std::cerr << "fileMappingCreate - CreateFileMapping failed, fname =  "
      << path_to_data << std::endl;
    CloseHandle(hFile);
    return;
  }

  unsigned char* dataPtr = (unsigned char*)MapViewOfFile(hMapping,
                                                         FILE_MAP_READ,
                                                         0,
                                                         0,
                                                         dwFileSize);

  if (dataPtr == nullptr) {
    std::cerr << "fileMappingCreate - MapViewOfFile failed, fname =  "
      << path_to_data << std::endl;
    CloseHandle(hMapping);
    CloseHandle(hFile);
    return;
  }

  UnmapViewOfFile(dataPtr);
  CloseHandle(hMapping);
  CloseHandle(hFile);
}

void Generate(const std::string& path, uintmax_t size, int min_int, int max_int) {
  std::ofstream out_file;
  out_file.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
  if (!out_file.is_open()) {
    std::cout << "couldn't open file\n";
    return;
  }
  //how much ints
  uintmax_t numb_count = size / 4;
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<int> uid(min_int, max_int);
  for (uintmax_t i = 0; i < numb_count; ++i) {
    int number = uid(gen);
    out_file.write(reinterpret_cast<const char*>(&number), sizeof(number));
  }
}

int main(int argc, char* argv[]) {
  std::string path_to_data;
  int thread_count = 1;
  int chunk_size_opt;
  int min_int = kMinInt;
  int max_int = std::numeric_limits<int>::max();
  bool chunk_size_present = false;
  po::options_description desc("General options");
  desc.add_options()
    ("min,l", po::value<int>(&min_int), "The minimum number to generate")
    ("max,r", po::value<int>(&max_int), "The maximum number to generate")
    ("gen,g", "Generate a file with random binary numbers")
    ("help,h", "Show help")
    ("file,f", po::value<std::string>(&path_to_data)->required(), "Input/output file with data")
    ("threads,t", po::value<int>(&thread_count), "How much threads is used to perform algorithm")
    ("chunk,c", po::value<int>(&chunk_size_opt), "Chunk/output file size in megabytes");
  po::variables_map vm;

  try {
    po::store(po::parse_command_line(argc, argv, desc), vm);

    if (vm.count("help")) {
      std::cout << desc << std::endl;
      return 0;
    }

    chunk_size_present = vm.count("chunk");

    po::notify(vm);
  } catch (...) {
    std::cerr << "error, usage: -h or --help" << std::endl;
    return 0;
  }

  if (vm.count("gen")) {
    uintmax_t file_size =
      (chunk_size_present ? chunk_size_opt : kDefaultOutputFileSizeMb) * kBytesInMb;
    Generate(path_to_data, file_size, min_int, max_int);
    return 0;
  }

  uintmax_t file_size = fs::file_size(path_to_data);
  int chunk_size = (chunk_size_present ? chunk_size_opt : 500) * kBytesInMb;

  ChunkRanges ranges;
  int chunk_count = static_cast<int>(file_size / chunk_size);
  for (int i = 0; i < chunk_count; ++i) {
    ranges.push_back(
      std::pair<uintmax_t, uintmax_t>(i * chunk_size, chunk_size));
  }
  uintmax_t tail = file_size - chunk_count * chunk_size;
  if (tail > 0) {
    ranges.push_back(
      std::pair<uintmax_t, uintmax_t>(chunk_count * chunk_size, tail));
  }

  DWORD start_tick = ::GetTickCount();
  int max_numb = kMinInt;
  for (size_t i = 0; i < ranges.size(); ++i) {
    int max_in_chunk =
      boost_func(path_to_data, thread_count, ranges[i].first, ranges[i].second);
    if (max_in_chunk > max_numb) {
      max_numb = max_in_chunk;
    }
  }
  std::cout << "The maximum number is " << max_numb << std::endl;
  std::cout << "Execution time: " << ::GetTickCount() - start_tick << " ticks";
}