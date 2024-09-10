#include <iomanip>
#include <utility>
#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/rtree.h"
#include "cista/serialization.h"
#include "cista/type_hash/static_type_hash.h"
#endif


// HELPER FUNCTIONS
/*
struct string_data_type {
  std::string test_string;
  bool operator==(string_data_type& other_struct){
    bool result;
    result = (other_struct.test_string == test_string);
    return result;
  }


  string_data_type() = default;

  ~string_data_type() = default;

  string_data_type(const string_data_type& other) = default; // copy constructor

  string_data_type(string_data_type&& other) = default; // move constructor


  string_data_type& operator=(const string_data_type& other) = default; //copy assignment


  string_data_type& operator=(string_data_type&& other)  noexcept { //move assignment
    //std::cout << test_string << "\n";
    test_string = other.test_string;
    return *this;
  }

};*/

void fill_rand_rect(std::vector<cista::rtree<size_t>::rect> &rand_vector) {

  float min_x = (float(rand())/float((RAND_MAX)) * 360) - 180;
  float min_y = (float(rand())/float((RAND_MAX)) * 180) - 90;
    cista::rtree<size_t>::rect rand_rect = {{min_x, min_y}, {min_x + (float(rand())/float((RAND_MAX)) * 2), min_y + (float(rand())/float((RAND_MAX)) * 2)}};
    rand_vector.emplace_back(rand_rect);
}

/*
void fill_rand_rect_string(std::vector<cista::rtree<string_data_type>::rect> &rand_vector) {

  float min_x = (float(rand())/float((RAND_MAX)) * 360) - 180;
  float min_y = (float(rand())/float((RAND_MAX)) * 180) - 90;
  cista::rtree<string_data_type>::rect rand_rect = {{min_x, min_y}, {min_x + (float(rand())/float((RAND_MAX)) * 2), min_y + (float(rand())/float((RAND_MAX)) * 2)}};
  rand_vector.emplace_back(rand_rect);
}*/

void print_node(cista::rtree<size_t> rt, cista::rtree<size_t>::node_idx_t node_id, bool complete) {
  std::cout << node_id << " [shape=record, label=\"{";
  std::stringstream children;
  std::stringstream rect_string;
  std::vector<cista::rtree<size_t>::node_idx_t> nodes_to_visit;

  for (size_t i = 0;/*rt.root_ != cista::rtree<size_t>::node_idx_t::invalid() &&*/ i < rt.nodes_[node_id].count_; ++i) {
    rect_string << "|{";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].min_[0] << ", ";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].min_[1] << " / ";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].max_[0] << ", ";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].max_[1] << "}";


    if (rt.nodes_[node_id].kind_ == cista::rtree<size_t>::kind::kBranch) {
      children << node_id << " -> " << rt.nodes_[node_id].children_[i] << "\n";
      nodes_to_visit.emplace_back(rt.nodes_[node_id].children_[i]);
    }
  }
  std::string output_rect_string = "rectangles";
  if (complete) {
    output_rect_string = rect_string.str().substr(1);
  }

  std::cout << "{id: " << node_id << ", count: " << rt.get_node(node_id).count_ << ", kind: " << (rt.get_node(node_id).kind_ == cista::rtree<size_t>::kind::kLeaf ? "leaf" : "branch") << "}|{" << output_rect_string << "}";
  std::cout << "}\"]" << "\n";
  std::cout << children.str();
  for (auto i : nodes_to_visit) {
    print_node(rt, i, complete);
  }
}

void print_visualize(cista::rtree<size_t> &rt, bool complete = false) {
  std::cout << "digraph rtree {\n";
  if (rt.root_ != cista::rtree<size_t>::node_idx_t::invalid()) {
    print_node(rt, rt.root_, complete);
  }
  std::cout << "}\n";
}

/*
void print_node_string(cista::rtree<string_data_type> rt, cista::rtree<string_data_type>::node_idx_t node_id, bool complete) {
  std::cout << node_id << " [shape=record, label=\"{";
  std::stringstream children;
  std::stringstream rect_string;
  std::vector<cista::rtree<size_t>::node_idx_t> nodes_to_visit;

  for (size_t i = 0; i < rt.nodes_[node_id].count_; ++i) {
    if (rt.nodes_[node_id].kind_ == cista::rtree<string_data_type>::kind::kLeaf) {
      rect_string << "|{";
      rect_string << rt.nodes_[node_id].data_[i].test_string << "}";
    } else {

      rect_string << "|{";
      rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].min_[0] << ", ";
      rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].min_[1] << " / ";
      rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].max_[0] << ", ";
      rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].max_[1] << "}";

      children << node_id << " -> " << rt.nodes_[node_id].children_[i] << "\n";
      nodes_to_visit.emplace_back(rt.nodes_[node_id].children_[i]);
    }
  }
  std::string output_rect_string = "rectangles";
  if (complete) {
    output_rect_string = rect_string.str().substr(1);
  }

  std::cout << "{id: " << node_id << ", count: " << rt.get_node(node_id).count_ << ", kind: " << (rt.get_node(node_id).kind_ == cista::rtree<string_data_type>::kind::kLeaf ? "leaf" : "branch") << "}|{" << output_rect_string << "}";
  std::cout << "}\"]" << "\n";
  std::cout << children.str();
  for (auto i : nodes_to_visit) {
    print_node_string(rt, i, complete);
  }
}*/

/*
void print_visualize_string(cista::rtree<string_data_type> &rt, bool complete = false) {
  std::cout << "digraph rtree {\n";
  if (rt.root_ != cista::rtree<string_data_type>::node_idx_t::invalid()) {
    print_node_string(rt, rt.root_, complete);
  }
  std::cout << "}\n";
}*/





TEST_SUITE("rtree") {

  TEST_CASE("sort by axis") {
    int N = 10;

    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    for (int i = 0; i < N; ++i) {
      fill_rand_rect(rand_rects_list);
    }

    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);
    }

    rt.get_node(rt.root_).sort_by_axis(0, true, true);
    bool sorted = true;
    for (size_t i = 0; i < rt.get_node(rt.root_).count_ - 1; ++i) {
      if (rt.get_node(rt.root_).rects_[i].min_[0] < rt.get_node(rt.root_).rects_[i + 1].min_[0]) {
        sorted = false;
      }
    }
    CHECK(sorted);
  }

  TEST_CASE("new node") {
    int N = 10;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    for (int i = 0; i < N; ++i) {
      fill_rand_rect(rand_rects_list);
    }

    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);
    }

    CHECK((rt.nodes_.size() == 1U));
    rt.node_new(cista::rtree<size_t>::kind::kLeaf);
    CHECK((rt.nodes_.size() == 2U));
  }

  TEST_CASE("move from node to node") {
    int N = 10;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    for (int i = 0; i < N; ++i) {
      fill_rand_rect(rand_rects_list);
    }

    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);
    }

    cista::rtree<size_t>::node_idx_t other_node;
    other_node = rt.node_new(cista::rtree<size_t>::kind::kLeaf);
    rt.get_node(rt.root_).move_rect_at_index_into(5, rt.get_node(other_node));
    rt.get_node(rt.root_).move_rect_at_index_into(5, rt.get_node(other_node));
    rt.get_node(rt.root_).move_rect_at_index_into(5, rt.get_node(other_node));
    CHECK((rt.get_node(other_node).count_ == 3));
    CHECK((rt.get_node(rt.root_).count_ == 7));
  }

  TEST_CASE("random insert") {
    int N = 100000;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(nullptr)));

    for (int i = 0; i < N; ++i) {
      fill_rand_rect(rand_rects_list);
    }

    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);
    }
    //print_visualize(rt);

    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      bool found_correct = false;

      rt.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK(found_correct);
    }
  }

  TEST_CASE("simple delete: depth 1") {
    int N = 10;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(nullptr)));

    for (int i = 0; i < N; ++i) {
      fill_rand_rect(rand_rects_list);
    }



    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);

    }

    size_t index = 5;
    cista::rtree<size_t>::coord_t min = rand_rects_list[index].min_;
    cista::rtree<size_t>::coord_t max = rand_rects_list[index].max_;
    bool found_correct = false;

    rt.search(min, max, [min, max, index, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
      if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == index) {
        found_correct = true;
      }
      return true;
    });

    CHECK((found_correct));

    rt.delete_element(min, max, index);

    found_correct = false;
    rt.search(min, max, [min, max, index, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
      if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == index) {
        found_correct = true;
      }
      return true;
    });

    CHECK((!found_correct));

  }

  TEST_CASE("simple delete: depth 2") {
    int N = 70;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(nullptr)));

    if (true) {
      for (int i = 0; i < N; ++i) {
        fill_rand_rect(rand_rects_list);
      }
    }

    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);

    }

    cista::rtree<size_t>::node_idx_t temp_id;
    temp_id.v_ = 2;
    size_t index = rt.get_node(temp_id).data_[1];
    cista::rtree<size_t>::coord_t min = rand_rects_list[index].min_;
    cista::rtree<size_t>::coord_t max = rand_rects_list[index].max_;
    bool found_correct = false;

    rt.search(min, max, [min, max, index, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
      if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == index) {
        found_correct = true;
      }
      return true;
    });

    CHECK((found_correct));

    //print_visualize(rt, true);

    rt.delete_element(min, max, index);

    //print_visualize(rt, true);

    found_correct = false;
    rt.search(min, max, [min, max, index, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
      if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == index) {
        found_correct = true;
      }
      return true;
    });

    CHECK((!found_correct));

  }

  TEST_CASE("delete node") {
    int N = 70;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(nullptr)));

    if (true) {
      for (int i = 0; i < N; ++i) {
        fill_rand_rect(rand_rects_list);
      }
    }

    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);

    }


    //print_visualize(rt, true);
    for (int i = static_cast<int>(rt.get_node(rt.get_node(rt.root_).children_[0]).count_ - 1); i >= 0; --i) {
      auto& temp_rect = rt.get_node(rt.get_node(rt.root_).children_[0]).rects_[static_cast<size_t>(i)];
      auto& temp_data = rt.get_node(rt.get_node(rt.root_).children_[0]).data_[static_cast<size_t>(i)];

      bool found_correct = false;

      rt.search(temp_rect.min_, temp_rect.max_, [temp_rect, temp_data, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(temp_rect.min_, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(temp_rect.max_, max_temp) && data == temp_data) {
          found_correct = true;
        }
        return true;
      });

      CHECK((found_correct));

      rt.delete_element(temp_rect.min_, temp_rect.max_, temp_data);

      found_correct = false;

      rt.search(temp_rect.min_, temp_rect.max_, [temp_rect, temp_data, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(temp_rect.min_, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(temp_rect.max_, max_temp) && data == temp_data) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }

    //print_visualize(rt, true);
  }

  TEST_CASE("delete root") {
    int N = 70;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(nullptr)));

    if (true) {
      for (int i = 0; i < N; ++i) {
        fill_rand_rect(rand_rects_list);
      }
    }

    cista::rtree<size_t> rt;
    //print_visualize(rt);
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);

    }


    //print_visualize(rt, true);
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      bool found_correct = false;

      rt.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });


      CHECK((found_correct));

      rt.delete_element(rand_rects_list[i].min_, rand_rects_list[i].max_, i);

      found_correct = false;

      rt.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }

    //print_visualize(rt, true);
  }

  TEST_CASE("insert after delete") {
    int N = 10000;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(nullptr)));

    if (true) {
      for (int i = 0; i < N; ++i) {
        fill_rand_rect(rand_rects_list);
      }
    }

    cista::rtree<size_t> rt;
    //print_visualize(rt);
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);

    }

    cista::rtree<size_t>::node_idx_t temp_id;
    temp_id.v_ = 2;
    /*
    size_t index = rt.get_node(temp_id).data_[1];
    cista::rtree<size_t>::coord_t min = rand_rects_list[index].min_;
    cista::rtree<size_t>::coord_t max = rand_rects_list[index].max_;

     */


    //print_visualize(rt, true);
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      bool found_correct = false;

      rt.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });


      CHECK((found_correct));

      rt.delete_element(rand_rects_list[i].min_, rand_rects_list[i].max_, i);

      found_correct = false;

      rt.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }

    uint32_t tree_size_after_deletion = rt.nodes_.size();

    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);

      bool found_correct = false;

      rt.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });


      CHECK((found_correct));
    }

    CHECK((tree_size_after_deletion == rt.nodes_.size()));

    //print_visualize(rt, true);
  }

  TEST_CASE("custom data struct") {
    uint32_t N = 1000;
    srand(static_cast<unsigned>(time(nullptr)));

    struct custom_data_type {
      uint32_t integer_data;
      std::array<uint32_t, 10> array_data;
      uint32_t *int_array_data;


      bool operator==(custom_data_type& other_struct){
        bool result;
        result = (other_struct.integer_data == integer_data);

        for (size_t i = 0; i < array_data.size(); ++i) {
          result = result && (array_data[i] == other_struct.array_data[i]);
        }
        return result;
      }

    };



    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    std::vector<custom_data_type> data_input_list;
    if (true) {
      for (uint32_t i = 0; i < N; ++i) {
        fill_rand_rect(rand_rects_list);

        custom_data_type input_data{};
        input_data.integer_data = i;
        //input_data.string_data = std::to_string(i);
        input_data.int_array_data =
            static_cast<uint32_t*>(malloc(sizeof(uint32_t) * i));
        for (uint32_t j = 0; j < input_data.array_data.size(); ++j) {
          input_data.array_data[j] = i + j;
        }
        data_input_list.emplace_back(input_data);
      }
    }

    cista::rtree<custom_data_type> rt;
    //print_visualize(rt);
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<custom_data_type>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<custom_data_type>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, data_input_list[i]);

    }


    //print_visualize(rt, true);
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<custom_data_type>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<custom_data_type>::coord_t max = rand_rects_list[i].max_;
      bool found_correct = false;

      rt.search(min, max, [min, max, i, &data_input_list, &found_correct](cista::rtree<custom_data_type>::coord_t const& min_temp, cista::rtree<custom_data_type>::coord_t const& max_temp, custom_data_type data){
        if (cista::rtree<custom_data_type>::rect::coord_t_equal(min, min_temp) && cista::rtree<custom_data_type>::rect::coord_t_equal(max, max_temp) && data == data_input_list[i]) {
          found_correct = true;
        }
        return true;
      });


      CHECK((found_correct));

      rt.delete_element(rand_rects_list[i].min_, rand_rects_list[i].max_, data_input_list[i]);

      found_correct = false;

      rt.search(min, max, [min, max, i, &data_input_list, &found_correct](cista::rtree<custom_data_type>::coord_t const& min_temp, cista::rtree<custom_data_type>::coord_t const& max_temp, custom_data_type data){
        if (cista::rtree<custom_data_type>::rect::coord_t_equal(min, min_temp) && cista::rtree<custom_data_type>::rect::coord_t_equal(max, max_temp) && data == data_input_list[i]) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }

    uint32_t tree_size_after_deletion = rt.nodes_.size();

    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<custom_data_type>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<custom_data_type>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, data_input_list[i]);

      bool found_correct = false;

      rt.search(min, max, [min, max, i, &data_input_list, &found_correct](cista::rtree<custom_data_type>::coord_t const& min_temp, cista::rtree<custom_data_type>::coord_t const& max_temp, custom_data_type data){
        if (cista::rtree<custom_data_type>::rect::coord_t_equal(min, min_temp) && cista::rtree<custom_data_type>::rect::coord_t_equal(max, max_temp) && data == data_input_list[i]) {
          found_correct = true;
        }
        return true;
      });


      CHECK((found_correct));
    }

    CHECK((tree_size_after_deletion == rt.nodes_.size()));
    //std::cout << "rt.nodes_.size(): " << std::to_string(tree_size_after_deletion * sizeof(cista::rtree<custom_data_type>::node) ) << "\n";

    //print_visualize(rt, true);
  }

  /*

  // This test has Problems with inserting std::string into the R-tree. Changing
  // the union in node from rtree.h to normal members does not help. I suspect a
  // custom move constructor or similar is necessary but was not able to write a
  // working one.
  TEST_CASE("string data struct") {

    uint32_t N = 100;
    srand(static_cast<unsigned>(time(nullptr)));

    std::vector<cista::rtree<string_data_type>::rect> rand_rects_list;
    std::vector<string_data_type> data_input_list;
    if (true) {
      for (uint32_t i = 0; i < N; ++i) {
        fill_rand_rect_string(rand_rects_list);

        auto* input_data = new string_data_type();
        input_data->test_string = std::to_string(i);
        data_input_list.emplace_back(*input_data);
      }
    }

    for (uint32_t i = 0; i < N; ++i) {
      //std::cout << data_input_list[i].test_string << "\n";
    }

    cista::rtree<string_data_type> rt;
    //print_visualize(rt);
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<string_data_type>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<string_data_type>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, data_input_list[i]);

    }


    print_visualize_string(rt, true);

    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<string_data_type>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<string_data_type>::coord_t max = rand_rects_list[i].max_;
      bool found_correct = false;

      rt.search(min, max, [min, max, i, &data_input_list, &found_correct](cista::rtree<string_data_type>::coord_t const& min_temp, cista::rtree<string_data_type>::coord_t const& max_temp, string_data_type data){
        //std::cout <<
        if (cista::rtree<string_data_type>::rect::coord_t_equal(min, min_temp) && cista::rtree<string_data_type>::rect::coord_t_equal(max, max_temp) && data == data_input_list[i]) {
          found_correct = true;
        }
        return true;
      });

      //std::cout << "Checkpoint 1" << "\n";


      CHECK((found_correct));

      rt.delete_element(rand_rects_list[i].min_, rand_rects_list[i].max_, data_input_list[i]);

      found_correct = false;

      rt.search(min, max, [min, max, i, &data_input_list, &found_correct](cista::rtree<string_data_type>::coord_t const& min_temp, cista::rtree<string_data_type>::coord_t const& max_temp, string_data_type data){
        if (cista::rtree<string_data_type>::rect::coord_t_equal(min, min_temp) && cista::rtree<string_data_type>::rect::coord_t_equal(max, max_temp) && data == data_input_list[i]) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }

    uint32_t tree_size_after_deletion = rt.nodes_.size();

    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<string_data_type>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<string_data_type>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, data_input_list[i]);

      bool found_correct = false;

      rt.search(min, max, [min, max, i, &data_input_list, &found_correct](cista::rtree<string_data_type>::coord_t const& min_temp, cista::rtree<string_data_type>::coord_t const& max_temp, string_data_type data){
        if (cista::rtree<string_data_type>::rect::coord_t_equal(min, min_temp) && cista::rtree<string_data_type>::rect::coord_t_equal(max, max_temp) && data == data_input_list[i]) {
          found_correct = true;
        }
        return true;
      });


      CHECK((found_correct));
    }

    CHECK((tree_size_after_deletion == rt.nodes_.size()));
    std::cout << "rt.nodes_.size(): " << std::to_string(tree_size_after_deletion * sizeof(cista::rtree<string_data_type>::node) ) << "\n";


    //print_visualize_string(rt, true);
  }
     */

  TEST_CASE("multiple dims rectangle") {
    int N = 1000;
    srand(static_cast<unsigned>(time(nullptr)));

    // 1D
    const uint32_t dims_1D = 1;
    std::vector<cista::rtree<size_t, dims_1D>::rect> rand_rects_list_1D;

    for (int i = 0; i < N; ++i) {
      cista::rtree<size_t, dims_1D>::rect rand_rect{};
      for (uint32_t j = 0; j < dims_1D; j++) {
        rand_rect.min_[j] = (float(rand())/float((RAND_MAX)) * 100) - 50;
        rand_rect.max_[j] = rand_rect.min_[j] + (float(rand())/float((RAND_MAX)) * 2);
      }
      rand_rects_list_1D.emplace_back(rand_rect);
    }

    cista::rtree<size_t, dims_1D> rt_1D;
    for (size_t i = 0; i < rand_rects_list_1D.size(); ++i) {
      cista::rtree<size_t, dims_1D>::coord_t min = rand_rects_list_1D[i].min_;
      cista::rtree<size_t, dims_1D>::coord_t max = rand_rects_list_1D[i].max_;
      rt_1D.insert(min, max, i);
    }
    //print_visualize(rt);

    for (size_t i = 0; i < rand_rects_list_1D.size(); ++i) {
      cista::rtree<size_t, dims_1D>::coord_t min = rand_rects_list_1D[i].min_;
      cista::rtree<size_t, dims_1D>::coord_t max = rand_rects_list_1D[i].max_;
      bool found_correct = false;

      rt_1D.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t, dims_1D>::coord_t const& min_temp, cista::rtree<size_t, dims_1D>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t, dims_1D>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t, dims_1D>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK(found_correct);

      rt_1D.delete_element(rand_rects_list_1D[i].min_, rand_rects_list_1D[i].max_, i);

      found_correct = false;

      rt_1D.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t, dims_1D>::coord_t const& min_temp, cista::rtree<size_t, dims_1D>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t, dims_1D>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t, dims_1D>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }

    // 3D
    const uint32_t dims_3D = 3;
    std::vector<cista::rtree<size_t, dims_3D>::rect> rand_rects_list_3D;

    for (int i = 0; i < N; ++i) {
      cista::rtree<size_t, dims_3D>::rect rand_rect{};
      for (uint32_t j = 0; j < dims_3D; j++) {
        rand_rect.min_[j] = (float(rand())/float((RAND_MAX)) * 100) - 50;
        rand_rect.max_[j] = rand_rect.min_[j] + (float(rand())/float((RAND_MAX)) * 2);
      }
      rand_rects_list_3D.emplace_back(rand_rect);
    }

    cista::rtree<size_t, dims_3D> rt_3D;
    for (size_t i = 0; i < rand_rects_list_1D.size(); ++i) {
      cista::rtree<size_t, dims_3D>::coord_t min = rand_rects_list_3D[i].min_;
      cista::rtree<size_t, dims_3D>::coord_t max = rand_rects_list_3D[i].max_;
      rt_3D.insert(min, max, i);
    }
    //print_visualize(rt);

    for (size_t i = 0; i < rand_rects_list_1D.size(); ++i) {
      cista::rtree<size_t, dims_3D>::coord_t min = rand_rects_list_3D[i].min_;
      cista::rtree<size_t, dims_3D>::coord_t max = rand_rects_list_3D[i].max_;
      bool found_correct = false;

      rt_3D.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t, dims_3D>::coord_t const& min_temp, cista::rtree<size_t, dims_3D>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t, dims_3D>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t, dims_3D>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK(found_correct);

      rt_3D.delete_element(rand_rects_list_3D[i].min_, rand_rects_list_3D[i].max_, i);

      found_correct = false;

      rt_3D.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t, dims_3D>::coord_t const& min_temp, cista::rtree<size_t, dims_3D>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t, dims_3D>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t, dims_3D>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }

    // 8D
    const uint32_t dims_8D = 8;
    std::vector<cista::rtree<size_t, dims_8D>::rect> rand_rects_list_8D;

    for (int i = 0; i < N; ++i) {
      cista::rtree<size_t, dims_8D>::rect rand_rect{};
      for (uint32_t j = 0; j < dims_8D; j++) {
        rand_rect.min_[j] = (float(rand())/float((RAND_MAX)) * 100) - 50;
        rand_rect.max_[j] = rand_rect.min_[j] + (float(rand())/float((RAND_MAX)) * 2);
      }
      rand_rects_list_8D.emplace_back(rand_rect);
    }

    cista::rtree<size_t, dims_8D> rt_8D;
    for (size_t i = 0; i < rand_rects_list_1D.size(); ++i) {
      cista::rtree<size_t, dims_8D>::coord_t min = rand_rects_list_8D[i].min_;
      cista::rtree<size_t, dims_8D>::coord_t max = rand_rects_list_8D[i].max_;
      rt_8D.insert(min, max, i);
    }
    //print_visualize(rt);

    for (size_t i = 0; i < rand_rects_list_1D.size(); ++i) {
      cista::rtree<size_t, dims_8D>::coord_t min = rand_rects_list_8D[i].min_;
      cista::rtree<size_t, dims_8D>::coord_t max = rand_rects_list_8D[i].max_;
      bool found_correct = false;

      rt_8D.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t, dims_8D>::coord_t const& min_temp, cista::rtree<size_t, dims_8D>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t, dims_8D>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t, dims_8D>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK(found_correct);

      rt_8D.delete_element(rand_rects_list_8D[i].min_, rand_rects_list_8D[i].max_, i);

      found_correct = false;

      rt_8D.search(min, max, [min, max, i, &found_correct](cista::rtree<size_t, dims_8D>::coord_t const& min_temp, cista::rtree<size_t, dims_8D>::coord_t const& max_temp, size_t data){
        if (cista::rtree<size_t, dims_8D>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t, dims_8D>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });

      CHECK((!found_correct));
    }
  }
}
