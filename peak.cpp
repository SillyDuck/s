#include <iostream>
#include <fstream>
#include <numeric>
#include <algorithm>
#include <vector>
#include <chrono>
#include <climits>
#include <thread>
#include <mutex>
#include <set>

using namespace std;

class Solution {
public:
    Solution(vector<int>&& input, long long threshold) : inputData_(std::move(input)), threshold_(threshold){

    }
    void solveSingle(){
        int minLength = INT_MAX;
        int L = 0, R = 0;
        long long sum = 0;
        while(R < inputData_.size()){
            sum += inputData_[R];
            while(sum >= threshold_){
                if(R-L < minLength){
                    minLength = R-L;
                    ansIndexGold_ = {{L,R}};
                }else if(R-L == minLength){
                    ansIndexGold_.emplace(L,R);
                }
                sum -= inputData_[L];
                L++;
            }
            R++;
        } 
    }

    void spawnJobs(int numThread){
        vector<thread> threads;
        vector<thread> threadsPivot;

        int sz = inputData_.size();
        int patchSz = (sz - (sz%numThread ? sz%numThread : numThread) + numThread )/numThread;
        
        int start = 0;
        for (int i = 0; i < numThread; i++) {
            int end = start+patchSz;
            threads.push_back(thread( [=, this](){solvePar(start, min(end, sz));} ));
            if(i==numThread-1) continue;
            threadsPivot.push_back(thread( [=, this](){solveParPivot(start, min(sz,end+patchSz), end);} ));
            start += patchSz;
        }

        for (int i = 0; i < numThread; i++) {
            threads[i].join();
            if(i==numThread-1) continue;
            threadsPivot[i].join();
        }
    }

    void print(){
        cout << ansIndexGold_.size() << " " << ansIndex_.size() << endl;
        if(ansIndex_.size() < 100){
            for(auto& [l, r] : ansIndex_){
                cout << l << " " << r << " " << r - l+1;
                cout << endl;
            }
        }
        
        if(ansIndex_ == ansIndexGold_){
            cout << "Single thread result is eqaul to mutlithread result\n";
        }else{
            cout << "Something's wrong\n";
        }
    }

private:
    void solvePar(int start, int end){
        thread_local set<pair<int,int>> ansIndexInternal_;
        int L = start, R = start;
        long long sum = 0;
        
        while(R < end){
            sum += inputData_[R];
            
            while(sum >= threshold_){
                if(R-L < minLength_){
                    lock_guard<std::mutex> lock(mu_);
                    minLength_ = R-L;
                    ansIndexInternal_ = {{L,R}};
                }else if(R-L == minLength_){                    
                    ansIndexInternal_.emplace(L,R);
                }
                sum -= inputData_[L];
                L++;
            }
            R++;

        }
    }


    void solveParPivot(int start, int end, int pivot){
        thread_local set<pair<int,int>> ansIndexInternal_;
        int L = start, R = pivot;
        long long sum = reduce(inputData_.begin()+L, inputData_.begin()+R, 0UL);
        
        while(R < end){
            sum += inputData_[R];
            
            while(sum >= threshold_){
                if(R-L < minLength_){
                    minLength_ = R-L;
                    ansIndexInternal_ = {{L,R}};
                }else if(R-L == minLength_){
                    ansIndexInternal_.emplace(L,R);
                }
                sum -= inputData_[L];
                L++;
                if(L >= pivot){
                    return;
                }
            }
            R++;
        }
    }

    vector<int> inputData_;
    long long threshold_;
    set<pair<int,int>> ansIndexGold_;
    set<pair<int,int>> ansIndex_;
    
    std::mutex mu_;
    std::atomic<int> minLength_ = INT_MAX;

};

using std::chrono::high_resolution_clock;
using std::chrono::milliseconds;
using std::chrono::duration;
class Timer {
public:
    void reset(){
        last_ = high_resolution_clock::now();
    };

    void profile(const string_view& sv){
        auto now = high_resolution_clock::now();
        duration<double, std::milli> ms_double = now - last_;
        last_ = now;
        cout << sv << " used " << ms_double.count() << "ms\n";
    }
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> last_;
};

int main(int argc, char* argv[]){
    
    Timer t; t.reset();
    
    ifstream ifs(argv[2]);
    if(!ifs.is_open()) return 0;
    vector<int> inputData;
    string str;
    while(ifs >> str){
        inputData.push_back(stoi(str));
    }
    t.profile("basic IO");
    
    long long threshold = atol(argv[1]);
    Solution s(std::move(inputData), threshold);
    s.solveSingle();

    t.profile("solve in 1 thread");

    int numThread = 16;
    s.spawnJobs(numThread);

    t.profile("solve in parallel");

    s.print();
}