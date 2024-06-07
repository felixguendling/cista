#include <iomanip>
#include "doctest.h"

#ifdef SINGLE_HEADER
#include "cista.h"
#else
#include "cista/containers/rtree.h"
#include "cista/serialization.h"
#include "cista/type_hash/static_type_hash.h"
#endif


// HELPER FUNCTIONS
void fill_rand_rect(std::vector<cista::rtree<size_t>::rect> &rand_vector) {

  float min_x = (float(rand())/float((RAND_MAX)) * 360) - 180;
  float min_y = (float(rand())/float((RAND_MAX)) * 180) - 90;
    cista::rtree<size_t>::rect rand_rect = {{min_x, min_y}, {min_x + (float(rand())/float((RAND_MAX)) * 2), min_y + (float(rand())/float((RAND_MAX)) * 2)}};
    rand_vector.emplace_back(rand_rect);
}
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





TEST_SUITE("rtree") {

  TEST_CASE("wip test") {
    using rtree_t = cista::rtree<void*>;

    cista::byte_buf buf;

    {
      rtree_t rt;
      for (auto i = 0.F; i != 10.F; ++i) {
        rt.insert({i, i}, {i + 1.F, i + 1.F}, nullptr);
      }
      buf = cista::serialize(rt);
    }  // EOL s

    auto const rt = cista::deserialize<rtree_t>(buf);
    /*
    for (auto i = 0.F; i != 10.F; ++i) {
      rt->search(
          {static_cast<float>(i + 0.F), static_cast<float>(i + 0.F)}, {static_cast<float>(i + 1.F), static_cast<float>(i + 1.F)},
          [i](rtree_t::coord_t const& min, rtree_t::coord_t const& max, void* data) {
            rtree_t::coord_t temp_min = {static_cast<float>(i), static_cast<float>(i)};
            rtree_t::coord_t temp_max = {static_cast<float>(i + 1.0), static_cast<float>(i + 1.0)};
            rtree_t::rect temp_rect = {temp_min, temp_max};
            std::cout << "i: " << i << ", min: " <<  ", (" << min[0] << ", " << min[1] << ") " << ", max: " << ", (" << max[0] << ", " << max[1] << ") " << ", temp_min: " <<  ", (" << temp_min[0] << ", " << temp_min[1] << ") " << ", temp_max: " << ", (" << temp_max[0] << ", " << temp_max[1] << ")\n";
            std::cout << "(static_cast<int>(min[0]) == static_cast<int>(temp_min[0])): " << (static_cast<int>(min[0]) == static_cast<int>(temp_min[0])) << "\n";
            std::cout << "min[0] " << std::to_string(min[0]) << " temp_min[0] " << std::to_string(temp_min[0]) << "\n";
            CHECK((data == nullptr));
            CHECK((min[0] == temp_min[0]));
            CHECK((min[1] == temp_min[1]));
            CHECK((max[0] == temp_max[0]));
            CHECK((max[1] == temp_max[1]));
            return true;
          });
    }

    rt->search({3.F, 3.F}, {7.F, 7.F},
               [](rtree_t::coord_t const& min, rtree_t::coord_t const& max, void* data) {
                 std::cout << "min: " <<  "(" << min[0] << ", " << min[1] << ") " << ", max: " << "(" << max[0] << ", " << max[1] << ") " << ")\n";
                 CHECK((data == nullptr));
                 return true;
               });*/

  }

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
    srand(static_cast<unsigned>(time(0)));

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
    srand(static_cast<unsigned>(time(0)));

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
    srand(static_cast<unsigned>(time(0)));

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
    srand(static_cast<unsigned>(time(0)));

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




    //print_visualize(rt, true);
    for (int i = static_cast<int>(rt.get_node(rt.get_node(rt.root_).children_[0]).count_ - 1); i >= 0; --i) {
      auto& temp_rect = rt.get_node(rt.get_node(rt.root_).children_[0]).rects_[i];
      auto& temp_data = rt.get_node(rt.get_node(rt.root_).children_[0]).data_[i];

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

    print_visualize(rt, true);
  }

  TEST_CASE("delete root") {
    int N = 70;
    std::vector<cista::rtree<size_t>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(0)));

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
    size_t index = rt.get_node(temp_id).data_[1];
    cista::rtree<size_t>::coord_t min = rand_rects_list[index].min_;
    cista::rtree<size_t>::coord_t max = rand_rects_list[index].max_;




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

    print_visualize(rt, true);
  }
}
