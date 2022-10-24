#pragma once


#include "Scene.hpp"
#include "Sound.hpp"
#include "data_path.hpp"

#include <glm/glm.hpp>
#include <stdint.h>
#include <list>
#include <vector>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <cassert>
#include <random>

#include <string>
#include <iostream>
#include <fstream>

namespace Gamel {

const glm::u8vec4 OPPONENT_COLOR = glm::u8vec4(0xf4, 0x04, 0x2c, 0x00);
const glm::u8vec4 CORRECT_COLOR = glm::u8vec4(0x44, 0xed, 0x69, 0x00);
const glm::u8vec4 DEFAULT_COLOR = glm::u8vec4(0xff, 0xff, 0xff, 0x00);
const glm::u8vec4 SELECTED_COLOR = glm::u8vec4(0x9f, 0x00, 0xcc, 0x00); 
constexpr char LOWER_TO_UPPER_SHIFT = 32;
constexpr float DEFAULT_WORD_LIFETIME = 6.0f;
constexpr float OPP_WORD_LIFETIME = DEFAULT_WORD_LIFETIME / 2.f;
constexpr float DEFAULT_WORD_X_POSITION = 1.75f;
constexpr float LETTER_WIDTH = 0.06f;
constexpr float TEXT_SIZE = 0.15f;
constexpr float HUD_SIZE = 0.11f;
constexpr float STEAL_MULTIPLIER = 1.5f;
constexpr uint32_t NEW_WORD_CYCLE = 4;
constexpr float GAME_LENGTH = 30.f;

constexpr float YPOS1 = 0.f;
constexpr float YPOS2 = 0.3f;
constexpr float YPOS3 = 0.8f;
constexpr float YPOS4 = -0.3f;
constexpr float YPOS5 = -0.8f;
constexpr float YPOS6 = -0.55f;
constexpr float YPOS7 = 0.55f;


enum Match {
    No, 
    Letter,
    WordSelf,
    WordOpp,
};

struct TrieNode {
    TrieNode(char c) {
        this->c = c;
        is_leaf = false; 
        is_steal = false;
        word = "";
    }
    char c;
    bool is_leaf;
    bool is_steal;
    std::string word;
    std::unordered_map<char, std::shared_ptr<TrieNode>> children;
    
};

struct Wordl {
    Wordl(const std::string& s, float lifetime, float y) {
        this->s = s;
        this->lifetime = lifetime;
        pos = glm::vec2(DEFAULT_WORD_X_POSITION, y);
        velo = -(s.length() * LETTER_WIDTH + 2 * DEFAULT_WORD_X_POSITION) / lifetime;
        remaining = 0.f;
        sent_to_opp = false;
    }
    float velo;
    glm::vec2 pos;
    float lifetime;
    float remaining;
    std::string s;
    bool sent_to_opp;
    bool move(float elapsed);
};

struct Playerl {
    
    Playerl() {
        root = std::make_shared<TrieNode>('\0');
        visited.emplace_back(root);
        available_ys.push_back(YPOS1);
        available_ys.push_back(YPOS2);
        available_ys.push_back(YPOS3);
        available_ys.push_back(YPOS4);
        available_ys.push_back(YPOS5);
        available_ys.push_back(YPOS6);
        available_ys.push_back(YPOS7);
        score = 0.f;
        oppscore = 0.f;
    }
    std::list<std::string> words_to_send_self;
    std::list<std::string> words_to_send_opp;
    std::list<std::shared_ptr<Wordl>> words;
    std::vector<std::shared_ptr<TrieNode>> visited;
    std::shared_ptr<TrieNode> root;


    void move_words(float elapsed);
    void add_word(const std::string& s, bool is_steal);
    void process_letter(char c); 
    bool has_word(const std::string& s);
    void remove_word(const std::string& s);
    float get_y();
    void return_y(float y);
    float score;
    float oppscore;
    private:
        Match match_letter(char c, std::string& completed);
        std::vector<float> available_ys;
        void remove_word_list(const std::vector<std::shared_ptr<TrieNode>>& words);
};



struct Gamel {

    Gamel() {
        dis = std::uniform_real_distribution(0.f, 1.f);
        std::string line;
        std::ifstream wordf;
        wordf.open(data_path("words.txt"));

        if(!wordf.is_open()) {
          perror("Error open");
          exit(EXIT_FAILURE);
        }
        while(getline(wordf, line)) {
            char *chars = const_cast<char *>(line.c_str());
            for (size_t i = 0; i < line.length(); ++i) {
                chars[i] = static_cast<char>(std::tolower(chars[i])); 
            }
            words.emplace(std::string(chars));
        }
        printf("TOTAL WORDS!: %zd\n", words.size());
    }
    std::unordered_set<std::string> words;

    std::default_random_engine e;
    std::uniform_real_distribution<float> dis; // rage 0 - 1
    std::string create_new_word();
    
    private:
};
}
