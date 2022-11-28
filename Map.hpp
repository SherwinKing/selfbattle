#pragma once

#include "Entity.hpp"

struct MapObject : Entity {
	MapObject() = default;	
	MapObject(float start_x, float start_y, SPRITE sprite_index) {
		x = start_x;
		y = start_y;
		this->sprite_index = sprite_index;
		tag = TAG::MAP_TAG;
	}

	MapObject(float start_x, float start_y, SPRITE sprite_index, BoundingBox box) {
		x = start_x;
		y = start_y;
		this->sprite_index = sprite_index;
		tag = TAG::MAP_TAG;
        this->box = box;
	}
};

const int average_num_per_section = 30;

class Map {
public:
    std::vector<MapObject> map_objects;

    // Collision detection used fields and methods

    float min_x = 0.f, max_x = 0.f;
	float map_width;

	int section_num;
	float section_width;
    float one_side_overlap;
	float section_width_with_overlap;
    std::vector<std::vector<MapObject>> sections;

    Map() = default;

    Map(std::vector<MapObject> map_objects) {
        this->map_objects = map_objects;

        // Get the horizontal boundary of the map
        for (auto & p : map_objects) {
            if (p.x < min_x) min_x = p.x;
            else if (p.x > max_x) max_x = p.x;
        }
        // Map horizontal size
        map_width = max_x - min_x;
        section_num= (int) (map_objects.size() / average_num_per_section);
        section_width = map_width / section_num;
        one_side_overlap = 0.2f * section_width;
        section_width_with_overlap = section_width + 2 * one_side_overlap;
        // Create the sections
        sections.resize(section_num);

        // Add the map object to the corresponding section
        for (auto p : map_objects) {
            int p_section_id = get_section_id(p.x, p.y);
            sections[p_section_id].push_back(p);
            if (is_in_section(p_section_id + 1, p.x, p.y)) {
                sections[p_section_id + 1].push_back(p);
            }
            if (is_in_section(p_section_id - 1, p.x, p.y)) {
                sections[p_section_id - 1].push_back(p);
            }
        }

    }

    inline int get_section_id(float x, float y) {
        if (x == max_x) return section_num - 1;
        float dx = x - min_x;
        int section_index = (int) (dx / section_width);
        return section_index;
    }

    inline bool is_in_section(int section_index, float x, float y) {
        if (section_index < 0 || section_index >= section_num) return false;
        float dx = x - min_x;
        float section_start = section_index * section_width - one_side_overlap;
        float section_end = section_start + section_width_with_overlap;
        return (dx >= section_start && dx <= section_end);
    }
};