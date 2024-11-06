#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <cctype>
#include <dirent.h>
#include <queue>
#include <iomanip>
#include <sstream>

class TextAnalyzer {
private:
    // These are the set of words to exclude
    const std::set<std::string> excludeWords = {"A", "AND", "AN", "OF", "IN", "THE"};
    

    std::vector<std::string> filePaths;
    std::vector<std::vector<double>> similarityMatrix;
    
    struct SimilarPair {
        int file1, file2;
        double similarity;
        
        bool operator<(const SimilarPair& other) const {
            return similarity < other.similarity;
        }
    };

    // Conversion of text to uppercase and clean it
    std::string normalizeText(const std::string& text) {
        std::string normalized;
        for (char c : text) {
            if (std::isalnum(c)) {
                normalized += std::toupper(c);
            } else {
                normalized += ' ';
            }
        }
        return normalized;
    }

    // This part of code gets word frequencies from text
    std::map<std::string, int> getWordFrequencies(const std::string& text) {
        std::map<std::string, int> frequencies;
        std::string word;
        int totalWords = 0;
        
        for (char c : text) {
            if (std::isspace(c)) {
                if (!word.empty()) {
                    if (excludeWords.find(word) == excludeWords.end()) {
                        frequencies[word]++;
                        totalWords++;
                    }
                    word.clear();
                }
            } else {
                word += c;
            }
        }
        
        // This part of code is used to normalize frequencies
        for (auto& pair : frequencies) {
            pair.second = (totalWords > 0) ? pair.second * 100 / totalWords : 0;
        }
        
        return frequencies;
    }

    // Get top 100 frequent words
    std::vector<std::pair<std::string, int>> getTop100Words(const std::map<std::string, int>& frequencies) {
        std::vector<std::pair<std::string, int>> words(frequencies.begin(), frequencies.end());
        std::sort(words.begin(), words.end(),
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        words.resize(std::min(size_t(100), words.size()));
        return words;
    }

    // This function is used to convert similarity to percentage
    double toPercentage(double similarity) {
        return similarity * 100.0;  // Conversion to percentage
    }


    double calculateSimilarity(const std::vector<std::pair<std::string, int>>& words1,
                             const std::vector<std::pair<std::string, int>>& words2) {
        int commonWords = 0;
        int totalUniqueWords = 0;
        std::map<std::string, bool> wordMap;

        // This is used to count common words and total unique words
        for (const auto& word1 : words1) {
            wordMap[word1.first] = true;
            for (const auto& word2 : words2) {
                if (word1.first == word2.first) {
                    commonWords++;
                    break;
                }
            }
        }

        // Add unique words from second file
        for (const auto& word2 : words2) {
            wordMap[word2.first] = true;
        }

        totalUniqueWords = wordMap.size();

        // Calculate percentage
        if (totalUniqueWords == 0) return 0.0;
        return (static_cast<double>(commonWords) / std::min(words1.size(), words2.size())) * 100.0;
    }

public:
    // Process a single file
    std::vector<std::pair<std::string, int>> processFile(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error opening file: " << filepath << std::endl;
            return {};
        }

        std::string content, line;
        while (std::getline(file, line)) {
            content += line + " ";
        }

        content = normalizeText(content);
        auto frequencies = getWordFrequencies(content);
        return getTop100Words(frequencies);
    }

    // Process all files and build similarity matrix
    void processAllFiles(const std::string& directory) {
        DIR* dir = opendir(directory.c_str());
        if (!dir) {
            std::cerr << "Error opening directory" << std::endl;
            return;
        }

        // Get all txt files
        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename.length() > 4 && 
                filename.substr(filename.length() - 4) == ".txt") {
                filePaths.push_back(directory + "/" + filename);
            }
        }
        closedir(dir);

        // Process files and build similarity matrix
        int n = filePaths.size();
        similarityMatrix.resize(n, std::vector<double>(n, 0.0));
        std::vector<std::vector<std::pair<std::string, int>>> processedFiles(n);

        std::cout << "Processing " << n << " files...\n";

        // Process all files
        for (int i = 0; i < n; i++) {
            processedFiles[i] = processFile(filePaths[i]);
            std::cout << "Processed file " << (i + 1) << "/" << n << "\r";
            std::cout.flush();
        }

        // Build similarity matrix
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                similarityMatrix[i][j] = similarityMatrix[j][i] = 
                    calculateSimilarity(processedFiles[i], processedFiles[j]);
            }
        }
    }

    void printTopSimilarPairs() {
        std::priority_queue<SimilarPair> pq;
        
        for (size_t i = 0; i < similarityMatrix.size(); i++) {
            for (size_t j = i + 1; j < similarityMatrix[i].size(); j++) {
                pq.push({static_cast<int>(i), static_cast<int>(j), similarityMatrix[i][j]});
            }
        }

        // This creates the output file
        std::ofstream outFile("similarity_results.txt");
        if (!outFile.is_open()) {
            std::cerr << "Error: Could not create output file" << std::endl;
            return;
        }

        // This part of the code is used to write to both console and file
        outFile << "Top 10 Similar Text Book Pairs\n";
        outFile << "============================\n\n";
        std::cout << "Top 10 Similar Text Book Pairs\n";
        std::cout << "============================\n\n";
        
        for (int i = 0; i < 10 && !pq.empty(); i++) {
            auto pair = pq.top();
            pq.pop();
            
            
            std::stringstream output;
            output << i + 1 << ". Similarity Score: " << std::fixed 
                   << std::setprecision(2) << pair.similarity << "%\n";
            output << "   Book 1: " << filePaths[pair.file1] << "\n";
            output << "   Book 2: " << filePaths[pair.file2] << "\n";
            output << "   ----------------------------------------\n\n";

            
            std::cout << output.str();
            outFile << output.str();
        }

        outFile.close();
        std::cout << "\nResults have been saved to 'similarity_results.txt'\n";
    }
};

int main() {
    TextAnalyzer analyzer;
    std::string directory = "./textbooks";  // This is directory containing the textbooks

    analyzer.processAllFiles(directory);
    analyzer.printTopSimilarPairs();

    return 0;
}

