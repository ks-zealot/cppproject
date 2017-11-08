#include <algorithm>
#include <clocale>
#include <condition_variable>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using namespace std;

class ThreadPool {
public:
  ThreadPool(int threads) : shutdown_(false) {
    // Create the specified number of threads
    threads_.reserve(threads);
    for (int i = 0; i < threads; ++i) {
      threads_.emplace_back(std::bind(&ThreadPool::threadEntry, this, i));
    }
  }

  ~ThreadPool() {
    {
      // Unblock any threads and tell them to stop
      std::unique_lock<std::mutex> l(lock_);

      shutdown_ = true;
      condVar_.notify_all();
    }

    // Wait for all threads to stop
    std::cerr << "Joining threads" << std::endl;
    for (auto &thread : threads_)
      thread.join();
  }

  void doJob(std::function<void(void)> func) {
    // Place a job on the queue and unblock a thread
    std::unique_lock<std::mutex> l(lock_);

    jobs_.emplace(std::move(func));
    condVar_.notify_one();
  }

protected:
  void threadEntry(int i) {
    std::function<void(void)> job;

    while (1) {
      {
        std::unique_lock<std::mutex> l(lock_);

        while (!shutdown_ && jobs_.empty())
          condVar_.wait(l);

        if (jobs_.empty()) {
          // No jobs to do and we are shutting down
          return;
        }

        job = std::move(jobs_.front());
        jobs_.pop();
      }

      // Do the job without holding any locks
      job();
    }
  }

  std::mutex lock_;
  std::condition_variable condVar_;
  bool shutdown_;
  std::queue<std::function<void(void)>> jobs_;
  std::vector<std::thread> threads_;
};

struct char_cont {
  unsigned short i = 0;
  unsigned short *arr;
  unsigned short size = 0;
};

std::vector<std::string> data;
std::vector<std::pair<int *, char_cont **>> encrypted;

void dump(int *counter, char_cont *data[], ostream &output) {

  for (int i = 0; i < *counter; i++) {
    if ((*data[i]).i > 0) {
      output << (*data[i]).i << endl;
    } else if ((*data[i]).arr) {
      for (int j = 0; j < (*data[i]).size; j++) {
        output << (*data[i]).arr[j];
        output << '|';
      }
      output << endl;
    }
  }
}

void encrypt(int *counter, string &str, char_cont *data[]) {
  unsigned int length = std::char_traits<char>::length(str.c_str());
  unsigned int strLen = str.length();
  unsigned int u = 0;
  const char *c_str = str.c_str();
  unsigned int charCount = 0;
  while (u < strLen) {
    int charsize = mblen(&c_str[u], strLen - u);
    if (charsize == 1) {
      char_cont *cont = new char_cont();
      cont->i = str[u] - '0';
      data[*counter] = cont;
    } else {
      char_cont *cont = new char_cont();
      cont->arr = new unsigned short[charsize];
      for (int i = 0; i < charsize; i++) {
        cont->arr[i] = str[u + i] - '0';
        cont->size++;
      }
      data[*counter] = cont;
    }
    u += charsize;
    (*counter)++;
    charCount += 1;
  }
}

int main(int args, char **argv) {
  setlocale(LC_ALL, "ru_RU.UTF-8");
  string ops(argv[1]);
  if (ops == "-e") {
    ThreadPool pool(1);
    ofstream output;
    output.open("1.crp");
    if (string(argv[2]) == "-f") {
      string ifn(argv[3]);
      ::ifstream ifs(ifn);
      if (ifs.is_open()) {
        string iLine;
        while (::std::getline(ifs, iLine)) {
          data.push_back(iLine);
        }
        for (std::vector<string>::iterator itr = data.begin();
             itr != data.end(); itr++) {
          string cur = *itr;
          unsigned int length = std::char_traits<char>::length(cur.c_str());
          char_cont **d = new char_cont *[length];
          int *counter = new int(0);
          pool.doJob(std::bind(encrypt, counter, cur, d));
          encrypted.push_back(std::make_pair(counter, d));
        }
        for (std::vector<pair<int *, char_cont **>>::iterator itr =
                 encrypted.begin();
             itr != encrypted.end(); itr++) {
          std::pair<int *, char_cont **> p = *itr;

          dump(p.first, p.second, output);
        }
      } else {
        ::cout << "could not open input file " << ::endl;
      }
    } else {
      for (int k = 2; k < args; k++) {
        string str(argv[k]);
        unsigned int length = std::char_traits<char>::length(str.c_str());
        char_cont **data = new char_cont *[length];
        int *counter = new int(0);
        encrypt(counter, str, data);
        dump(counter, data, output);
        for (int i = 0; i < *counter; i++) {
          delete[](*data[i]).arr;
        }
        delete[] data;
        output << 65520 << endl;
        cout << 65520 << endl;
      }
    }
    output.close();
  } else if (ops == "-d") {
    string line;
    ::ifstream input("1.crp");
    ostream *out;
    if (string(argv[2]) == "-f") {
      out = new ofstream(argv[3]);
    } else {
      out = &cout;
    }
    if (input.is_open()) {
      string decrypted = string();

      while (std::getline(input, line)) {
        size_t pos = line.find('|');
        if (pos > 0 && pos != string::npos) {
          char *symbol = new char[ ::count(line.begin(), line.end(), '|')];
          size_t prevPos = 0;
          size_t curPos = line.find('|');
          size_t count = 0;
          string curLine = line.substr(prevPos, curPos - prevPos);
          while (curLine.length() > 0) {
            symbol[count] = (unsigned char)(stoi(curLine) + 48);
            count++;
            prevPos = curPos + 1;
            curPos = line.find('|', prevPos);
            curLine = line.substr(prevPos, curPos - prevPos);
          }
          decrypted.append(string(symbol));
          *out << decrypted;
          decrypted = string();
          delete[] symbol;
        } else if (pos == string::npos) {
          if (line.length() > 0) {
            decrypted = decrypted.append(1, (unsigned char)(::stoi(line) + 48));
          } else {
            *out << decrypted << ' ';
            decrypted = string();
          }
        }
      }
      *out << endl;
      input.close();
    } else {
      cout << "Unable to open file" << endl;
    }
  } else {
    cout << "Unrecognized option " << ops << endl;
  }
  return 0;
}
