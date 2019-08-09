#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <streambuf>
#include <string>
#include <vector>
#include <tuple>
#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/date_time.hpp>
#include <boost/process.hpp>

// g++ converter.cpp -lboost_filesystem -lboost_system -lboost_program_options -o converter

using namespace std;
using namespace boost;
using namespace boost::program_options;
using namespace boost::filesystem;
using namespace boost::property_tree;
using namespace boost::posix_time;
using namespace boost::process;

int main(int argc,char ** argv) {
  string in_file;
  string audio_file;
  string out_file;
  options_description desc;
  desc.add_options()
    ("help,h", "print current message")
    ("input,i", value<string>(&in_file), "input json")
    ("audio,a", value<string>(&audio_file), "audio file")
    ("output,o", value<string>(&out_file), "output file");
  variables_map vm;
  store(parse_command_line(argc, argv, desc), vm);
  notify(vm);

  // process user options
  if (vm.count("help")) {
    cout<<desc;
    return EXIT_SUCCESS;
  } else if (1 != vm.count("input") || 1 != vm.count("audio") ||  1 != vm.count("output")) {
    cerr<<"input, output and audio file path must be given!"<<endl;
    return EXIT_FAILURE;
  } else if (false == exists(in_file) || true == is_directory(in_file)) {
    cerr<<"input json path is not valid!"<<endl;
    return EXIT_FAILURE;
  } else if (true == exists(out_file)) {
    cerr<<"output json file exists!"<<endl;
    return EXIT_FAILURE;
  }

  // load label json file
  std::ifstream in(in_file);
  if (false == in.is_open()) {
    cerr<<"couldn't open input json!"<<endl;
    return EXIT_FAILURE;
  }
  string input_json = string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  stringstream ss(input_json);
  ptree label;
  read_json(ss, label);

  // parse label file
  vector<std::tuple<float,float,string> > labels;
  float start_time_of_sentence;
  float end_time_of_sentence;
  string sentence;
  bool flag = true;
  for (auto& word: label.get_child("words")) {
    float start_time = lexical_cast<float>(word.second.get_child("startTime").get_value<string>());
    float end_time = lexical_cast<float>(word.second.get_child("endTime").get_value<string>());
    string value = word.second.get_child("value").get_value<string>();
    trim(value);
    if (flag == true) {
      start_time_of_sentence = start_time;
      flag = false;
      sentence = value;
    } else if (value == "." || value == ",") {
      sentence += value;
    } else {
      sentence += " " + value;
    }
    if (value == ".") {
      end_time_of_sentence = end_time;
      flag = true;
      labels.push_back(std::make_tuple(start_time_of_sentence, end_time_of_sentence, sentence));
    }
  }

  boost::filesystem::path out_root(out_file);
  create_directory(out_root);
  std::ofstream csv(out_file + ".csv");
  if (false == csv.is_open()) {
    cerr<<"can't open output csv file!"<<endl;
    return EXIT_FAILURE;
  }
  std::ofstream json(out_file + ".json");
  if (false == json.is_open()) {
    cerr<<"can't open output json file!"<<endl;
    return EXIT_FAILURE;
  }
  ptree list;
  int count = 0;
  // generate wave and label files.
  for (auto& label: labels) {
    float startTime = std::get<0>(label);
    float endTime = std::get<1>(label);
    string sentence = std::get<2>(label);
    auto ss = milliseconds(static_cast<int>(startTime * 1000));
    auto t = milliseconds(static_cast<int>(endTime * 1000) - static_cast<int>(startTime * 1000));
    vector<string> args;
    args.push_back("-i");
    args.push_back(audio_file);
    args.push_back("-ss");
    args.push_back(lexical_cast<string>(ss));
    args.push_back("-t");
    args.push_back(lexical_cast<string>(t));
    args.push_back("-acodec");
    args.push_back("pcm_s16le");
    args.push_back("-ar");
    args.push_back("44100");
    args.push_back((out_root / (lexical_cast<string>(count) + ".wav")).string());
    child c(search_path("ffmpeg"),args);
    c.wait();

    // write csv
    csv << count << "|" << sentence << "|" << sentence <<endl;

    // append json
    ptree transcription;
    transcription.put("original", sentence);
    transcription.put("clean", sentence);
    list.add_child(string_path<string, id_translator<string> >(lexical_cast<string>(count) + ".wav", '\\'), transcription);
  
    count++;
  }
  // write json
  stringstream buffer;
  write_json(buffer, list);
  json << buffer.str();
  csv.close();
  json.close();

  return EXIT_SUCCESS;
}

