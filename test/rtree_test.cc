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
void fill_rand_rect(std::vector<cista::rtree<void*>::rect> &rand_vector) {

  float min_x = (float(rand())/float((RAND_MAX)) * 360) - 180;
  float min_y = (float(rand())/float((RAND_MAX)) * 180) - 90;
    cista::rtree<void*>::rect rand_rect = {{min_x, min_y}, {min_x + (float(rand())/float((RAND_MAX)) * 2), min_y + (float(rand())/float((RAND_MAX)) * 2)}};
    rand_vector.emplace_back(rand_rect);
}
void print_node(cista::rtree<size_t> rt, cista::rtree<size_t>::node_idx_t node_id) {
  std::cout << node_id << " [shape=record, label=\"{";
  std::stringstream children;
  std::stringstream rect_string;
  std::vector<cista::rtree<size_t>::node_idx_t> nodes_to_visit;

  rect_string << "{";
  rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[0].min_[0] << ", ";
  rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[0].min_[1] << " / ";
  rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[0].max_[0] << ", ";
  rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[0].max_[1] << "}";

  for (size_t i = 1; i < rt.nodes_[node_id].count_ && rt.nodes_[node_id].children_[i] != cista::rtree<size_t>::node_idx_t::invalid(); ++i) {
    rect_string << "|{";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].min_[0] << ", ";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].min_[1] << " / ";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].max_[0] << ", ";
    rect_string << std::fixed << std::setprecision(2) << rt.nodes_[node_id].rects_[i].max_[1] << "}";

    if (false && rt.nodes_[node_id].children_[i] < rt.nodes_.size()) {
      /*
    }
    if (rt.get_node(rt.nodes_[node_id].children_[i]).kind_ == cista::rtree<size_t>::kind::kBranch) {*/
      children << node_id << " -> " << i << "\n";
      nodes_to_visit.emplace_back(rt.nodes_[node_id].children_[i]);
      //print_node(rt, rt.nodes_[node_id].children_[i]);
    }
  }
  std::cout << "{" << node_id << "}|{" << rect_string.str() << "}";
  std::cout << "}\"]" << "\n";
  std::cout << children.str();
  for (size_t i = 0; i < nodes_to_visit.size(); ++i) {
    std::cout << nodes_to_visit[i] << "\n";
    //print_node(rt, nodes_to_visit[i]);
  }
}

void print_visualize(cista::rtree<size_t> rt, int node_id = -1) {
  cista::rtree<size_t>::node_idx_t root;
  if (node_id != -1) {
    root.v_ = node_id;
  } else {
    root = rt.root_;
  }
  std::cout << "digraph rtree {\n";
  print_node(rt, root);
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
    }*/

    rt->search({3.F, 3.F}, {7.F, 7.F},
               [](rtree_t::coord_t const& min, rtree_t::coord_t const& max, void* data) {
                 //std::cout << "min: " <<  "(" << min[0] << ", " << min[1] << ") " << ", max: " << "(" << max[0] << ", " << max[1] << ") " << ")\n";
                 CHECK((data == nullptr));
                 return true;
               });

  }

  TEST_CASE("sort by axis") {
    int N = 10;
    std::vector<cista::rtree<void*>::rect> rand_rects_list;
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
    print_visualize(rt);
  }

  TEST_CASE("move from node to node") {
    int N = 10;
    std::vector<cista::rtree<void*>::rect> rand_rects_list;
    for (int i = 0; i < N; ++i) {
      fill_rand_rect(rand_rects_list);
    }

    cista::rtree<size_t> rt;
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);
    }

    cista::rtree<size_t>::node_idx_t other_node = rt.node_new(cista::rtree<size_t>::kind::kLeaf);
    rt.get_node(rt.root_).move_rect_at_index_into(5, rt.get_node(other_node));
    rt.get_node(rt.root_).move_rect_at_index_into(5, rt.get_node(other_node));
    rt.get_node(rt.root_).move_rect_at_index_into(5, rt.get_node(other_node));
    //print_visualize(rt, 0);
    //print_visualize(rt, 1);
  }

  TEST_CASE("random insert test") {
    int N = 65;
    std::vector<cista::rtree<void*>::rect> rand_rects_list;
    srand(static_cast<unsigned>(time(0)));
    for (int i = 0; i < N; ++i) {
      fill_rand_rect(rand_rects_list);
      //std::cout << "min: (" << rand_rects_list[i].min_[0] << ", " << rand_rects_list[i].min_[1] << "), max: (" << rand_rects_list[i].max_[0] << ", " << rand_rects_list[i].max_[1] << ")\n";
    }

    cista::rtree<size_t> rt;
    //std::cout << rt.root_.v_ << "\n";
    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      rt.insert(min, max, i);
    }

    std::cout << rt.root_.v_ << "\n";
    for (size_t i = 0; i < rt.nodes_.size(); ++i) {
      cista::rtree<size_t>::node_idx_t temp_node;
      temp_node.v_ = i;
      rt.get_node(temp_node);
      print_visualize(rt, i);
    }

    for (size_t i = 0; i < rand_rects_list.size(); ++i) {
      cista::rtree<size_t>::coord_t min = rand_rects_list[i].min_;
      cista::rtree<size_t>::coord_t max = rand_rects_list[i].max_;
      bool found_correct = false;
      int found_intersects = 0;
      rt.search(min, max, [min, max, i, &found_correct, &found_intersects](cista::rtree<size_t>::coord_t const& min_temp, cista::rtree<size_t>::coord_t const& max_temp, size_t data){

        //std::cout << "min: (" << min[0] << ", " << min[1] << "), min_temp: (" << min_temp[0] << ", " << min_temp[1] << ")\n";
        //std::cout << "max: (" << max[0] << ", " << max[1] << "), max_temp: (" << max_temp[0] << ", " << max_temp[1] << ")\n";
        //std::cout << "data: " << data << ", i: " << i << "\n";
        //std::cout << "min equal: " << cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) << ", max equal: " << cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) << "\n";

        found_intersects++;
        if (cista::rtree<size_t>::rect::coord_t_equal(min, min_temp) && cista::rtree<size_t>::rect::coord_t_equal(max, max_temp) && data == i) {
          found_correct = true;
        }
        return true;
      });
      if (found_intersects > 2) {
        //std::cout << "found_intersects: " << found_intersects << "\n";
      }
      //CHECK(found_correct);
    }
  }
}
