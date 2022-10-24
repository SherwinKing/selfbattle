#include "Gamel.hpp"

#include <ctime>
#include <cmath>

namespace Gamel {

float Playerl::get_y() {
    assert(!available_ys.empty());
    float res = available_ys[available_ys.size() - 1];
    available_ys.pop_back();
    return res;
}

void Playerl::return_y(float y) {
    available_ys.emplace_back(y);
}

void Playerl::add_word(const std::string& s, bool is_steal){
    float lifetime = is_steal ? OPP_WORD_LIFETIME : DEFAULT_WORD_LIFETIME;
    words.emplace_back(std::make_shared<Wordl>(s, lifetime, get_y()));
    std::shared_ptr<TrieNode> cur = root; 
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        auto it = cur->children.find(c);
        if (it == cur->children.end()) {
            auto new_node = std::make_shared<TrieNode>(c); 
            cur->children.emplace(c, new_node);
            cur = new_node;
        } 
        else {
            cur = it->second;
        }
        if (i == s.length() - 1) {
            cur->is_leaf = true; 
            cur->is_steal = is_steal;
            cur->word = s;
        }
    }       
};


bool Wordl::move(float elapsed) {
    if (elapsed + remaining > lifetime) {
        return false;
    }
    pos = glm::vec2(pos.x + elapsed * velo, pos.y);
    remaining += elapsed; 
    return true;
}

void Playerl::move_words(float elapsed) {
    std::list<std::shared_ptr<Wordl>> newlist;
    for (auto w : words) {
        if (w->move(elapsed)) {
            newlist.emplace_back(w); 
            if (w->lifetime == DEFAULT_WORD_LIFETIME && w->remaining > DEFAULT_WORD_LIFETIME / 2.f && !w->sent_to_opp){
                words_to_send_opp.emplace_back(w->s); 
                w->sent_to_opp = true;
            }
        }
        else {
            return_y(w->pos.y);
            if (w->lifetime == DEFAULT_WORD_LIFETIME) {
                words_to_send_self.emplace_back(w->s);
            }
            const std::string& s = w->s; 
			// Skip the first visited node which is always the root
			bool matching = true;
			for (size_t i = 1; i < visited.size(); ++i) {
				auto trienode = visited[i];
				if (i-1 >= s.length() || trienode->c != s[i-1]) {
					matching = false;	
					break;
				}
			}
            if (matching) {
                visited.clear(); 
                visited.emplace_back(root);
            }
        }
        
    }
    words = newlist; 
}

Match Playerl::match_letter(char c, std::string& completed) {
    auto curr = visited[visited.size()-1];
    auto it = curr->children.find(c);
    if (it == curr->children.end()) {
        return No; 
    }
    curr = it->second;
    visited.emplace_back(curr);
    if (!curr->is_leaf) {
        return Letter;
    }
    completed = curr->word;
    bool issteal = curr->is_steal;
    remove_word_list(visited); 
    auto it2 = words.begin();
    
    while (it2 != words.end()) {
        if ((*it2)->s ==  visited[visited.size()-1]->word) {
            return_y((*it2)->pos.y);
            words.erase(it2);
            break;
        }    
        it2++; 
    }
    visited.clear();
    visited.emplace_back(root);
    return issteal ? WordOpp : WordSelf;
}

void Playerl::process_letter(char c) {
    std::string completed = "";
    Match res = match_letter(c, completed); 
    if (res == Match::No) {
        // SFX?
    }
    else if (res == Match::Letter) {
        // SFX?
    }
    else if (res == Match::WordOpp) {
        // SFX?
        assert(completed != "");
        score += (static_cast<float>(completed.length()) * STEAL_MULTIPLIER);
        words_to_send_self.emplace_back(completed);
    }
    else if (res == Match::WordSelf) {
        // SFX?
        assert(completed != "");
        score += (static_cast<float>(completed.length()));
        words_to_send_self.emplace_back(completed);
    }
}

bool Playerl::has_word(const std::string& s) {
    std::shared_ptr<TrieNode> cur = root; 
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        auto it = cur->children.find(c);
        if (it == cur->children.end()) {
            return false;
        } 
        cur = it->second;
    } 
    return true;
}

void Playerl::remove_word(const std::string& s){
    std::vector<std::shared_ptr<TrieNode>> v;
    v.clear(); 
    v.emplace_back(root);
    std::shared_ptr<TrieNode> cur = root; 
    for (size_t i = 0; i < s.length(); ++i) {
        char c = s[i];
        auto it = cur->children.find(c);
        assert(it != cur->children.end());
        cur = it->second;
        v.emplace_back(cur);
    } 
    remove_word_list(v);
    auto it = words.begin();
    while (it != words.end()) {
        if ((*it)->s ==  v[v.size()-1]->word) {
            return_y((*it)->pos.y);
            words.erase(it);
            return;
        }    
        it++; 
    }
}

void Playerl::remove_word_list(const std::vector<std::shared_ptr<TrieNode>>& wordl) {
    int cur = static_cast<int>(wordl.size()) - 2; 
    while (cur >= 0 ) {
        auto node = wordl[cur];
        auto rem = wordl[cur+1];
        if (rem->children.empty()) {
            node->children.erase(rem->c); 
        }
        --cur;
    } 
}

std::string Gamel::create_new_word() {
    double random_val = (dis(e) * std::time(0) * dis(e)) ;
    size_t mod = static_cast<size_t>(random_val) % words.size();
    const auto& it = std::next(words.begin(), mod);
    std::string s = *it;
    words.erase(it); 
    return s;
}
}