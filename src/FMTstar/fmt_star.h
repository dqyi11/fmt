#ifndef FMTSTAR_H
#define FMTSTAR_H

#include <vector>
#include <list>

#include "KDTree2D.h"

typedef double (*COST_FUNC_PTR)(POS2D, POS2D, double**, void*);

class TreeNode {

public:
    TreeNode( POS2D pos );

    bool operator==( const TreeNode &other );

    double               m_cost;
    TreeNode*            mp_parent;
    POS2D                m_pos;
    std::list<TreeNode*> m_child_nodes;
};

class Path {

public:
    Path(POS2D start, POS2D goal);
    ~Path();

    double m_cost;
    POS2D  m_start;
    POS2D  m_goal;
    std::vector<POS2D> m_way_points;
};

class FMTstar {

public:
    FMTstar(int width, int height, int segment_length);
    ~FMTstar();

    TreeNode* init( POS2D start, POS2D goal, COST_FUNC_PTR p_func, double** pp_cost_distrinution );

    void load_map( int** pp_map );

    int get_sampling_width() { return _sampling_width; }
    int get_sampling_height() { return _sampling_height; }
    int get_current_iteration() { return _current_iteration; }

    std::list<TreeNode*>& get_nodes() { return _nodes; }

    int**& get_map_info() { return _pp_map_info; }
    double get_ball_radius() { return _ball_radius; }

    void extend();
    Path* find_path();

    void dump_distribution(std::string filename);

protected:
    POS2D _sampling();
    POS2D _steer( POS2D pos_a, POS2D pos_b );

    KDNode2D _find_nearest( POS2D pos );
    std::list<KDNode2D> _find_near( POS2D pos );

    bool _is_obstacle_free( POS2D pos_a, POS2D pos_b );
    bool _is_in_obstacle( POS2D pos );
    bool _contains( POS2D pos );

    double _calculate_cost( POS2D& pos_a, POS2D& pos_b );

    TreeNode* _create_new_node( POS2D pos );
    bool _remove_edge( TreeNode* p_node_parent, TreeNode* p_node_child );
    bool _has_edge( TreeNode* p_node_parent, TreeNode* p_node_child );
    bool _add_edge( TreeNode* p_node_parent, TreeNode* p_node_child );

    std::list<TreeNode*> _find_all_children( TreeNode* node );

    void _attach_new_node( TreeNode* p_node_new, TreeNode* p_nearest_node, std::list<TreeNode*> near_nodes );
    void _rewire_near_nodes( TreeNode* p_node_new, std::list<TreeNode*> near_nodes );
    void _update_cost_to_children( TreeNode* p_node, double delta_cost );
    bool _get_closet_to_goal( TreeNode*& p_node_closet_to_goal, double& delta_cost );

    TreeNode* _find_ancestor( TreeNode* p_node );

private:
    POS2D    _start;
    POS2D    _goal;
    TreeNode* _p_root;

    int _sampling_width;
    int _sampling_height;

    int** _pp_map_info;

    KDTree2D*     _p_kd_tree;
    COST_FUNC_PTR _p_cost_func;
    double**      _pp_cost_distribution;

    std::list<TreeNode*> _nodes;

    double _range;
    double _ball_radius;
    double _segment_length;
    int    _obs_check_resolution;

    double _theta;
    int    _current_iteration;
};

inline TreeNode* get_ancestor( TreeNode * node ) {
    if( NULL == node ) {
        return NULL;
    }
    if( NULL == node->mp_parent ) {
        return node;
    }
    else {
        return get_ancestor( node->mp_parent );
    }
}

inline void get_parent_node_list( TreeNode * node, std::list<TreeNode*>& path ) {
    if( node==NULL ) {
        return;
    }
    path.push_back( node );
    get_parent_node_list( node->mp_parent, path );
    return;
}

#endif // FMTSTAR_H
