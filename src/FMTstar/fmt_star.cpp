#include <limits>
#include <iostream>
#include <fstream>

#include "fmt_star.h"

#define OBSTACLE_THRESHOLD 200

TreeNode::TreeNode(POS2D pos) {
    m_pos = pos;
    m_cost = 0.0;
    mp_parent = NULL;
}

bool TreeNode::operator==(const TreeNode &other) {
    return m_pos==other.m_pos;
}

Path::Path(POS2D start, POS2D goal) {
    m_start = start;
    m_goal = goal;
    m_cost = 0.0;
}

Path::~Path() {
    m_cost = 0.0;
}

FMTstar::FMTstar( int width, int height, int segment_length ) {

    _sampling_width = width;
    _sampling_height = height;
    _segment_length = segment_length;
    _p_root = NULL;

    _p_kd_tree = new KDTree2D( std::ptr_fun(tac) );

    _range = (_sampling_width > _sampling_height) ? _sampling_width:_sampling_height;
    _ball_radius = _range;
    _obs_check_resolution = 1;
    _current_iteration = 0;
    _segment_length = segment_length;

    _theta = 10;

    _pp_cost_distribution = NULL;

    _pp_map_info = new int*[_sampling_width];
    for(int i=0;i<_sampling_width;i++) {
        _pp_map_info[i] = new int[_sampling_height];
        for(int j=0;j<_sampling_height;j++) {
            _pp_map_info[i][j] = 255;
        }
    }

    _nodes.clear();
}

FMTstar::~FMTstar() {
    if(_p_kd_tree) {
        delete _p_kd_tree;
        _p_kd_tree = NULL;
    }
}

TreeNode* FMTstar::init( POS2D start, POS2D goal, COST_FUNC_PTR p_func, double** pp_cost_distribution ) {
    if( _p_root ) {
        delete _p_root;
        _p_root = NULL;
    }
    _start = start;
    _goal = goal;
    _p_cost_func = p_func;

    if(pp_cost_distribution) {
        if(_pp_cost_distribution == NULL) {
            _pp_cost_distribution = new double*[_sampling_width];
            for(int i=0;i<_sampling_width;i++) {
                _pp_cost_distribution[i] = new double[_sampling_height];
            }
        }
        for(int i=0;i<_sampling_width;i++) {
            for(int j=0;j<_sampling_height;j++) {
                _pp_cost_distribution[i][j] = pp_cost_distribution[i][j];
            }
        }
    }
    else {
        if(_pp_cost_distribution) {
            _pp_cost_distribution = NULL;
        }
    }

    KDNode2D root( start );

    _p_root = new TreeNode( start );
    _nodes.push_back(_p_root);
    root.setTreeNode(_p_root);

    _p_kd_tree->insert( root );
    _current_iteration = 0;

    return _p_root;
}

void FMTstar::load_map( int** pp_map ) {
    for(int i=0;i<_sampling_width;i++) {
        for(int j=0;j<_sampling_height;j++) {
            _pp_map_info[i][j] = pp_map[i][j];
        }
    }
}

POS2D FMTstar::_sampling() {
    double x = rand();
    double y = rand();
    int int_x = x * ((double)(_sampling_width)/RAND_MAX);
    int int_y = y * ((double)(_sampling_height)/RAND_MAX);

    POS2D m(int_x,int_y);
    return m;
}

POS2D FMTstar::_steer( POS2D pos_a, POS2D pos_b ) {
    POS2D new_pos( pos_a[0], pos_a[1] );
    double delta[2];
    delta[0] = pos_a[0] - pos_b[0];
    delta[1] = pos_a[1] - pos_b[1];
    double delta_len = sqrt(delta[0]*delta[0]+delta[1]*delta[1]);

    if (delta_len > _segment_length) {
        double scale = _segment_length / delta_len;
        delta[0] = delta[0] * scale;
        delta[1] = delta[1] * scale;

        new_pos.setX( pos_b[0]+delta[0] );
        new_pos.setY( pos_b[1]+delta[1] );
    }
    return new_pos;
}

bool FMTstar::_is_in_obstacle( POS2D pos ) {
    int x = (int)pos[0];
    int y = (int)pos[1];
    if( _pp_map_info[x][y] < 255 ) {
        return true;
    }
    return false;
}


bool FMTstar::_is_obstacle_free( POS2D pos_a, POS2D pos_b ) {
    if ( pos_a == pos_b ) {
        return true;
    }
    int x_dist = pos_a[0] - pos_b[0];
    int y_dist = pos_a[1] - pos_b[1];

    if( x_dist == 0 && y_dist == 0) {
        return true;
    }

    float x1 = pos_a[0];
    float y1 = pos_a[1];
    float x2 = pos_b[0];
    float y2 = pos_b[1];

    const bool steep = ( fabs(y2 - y1) > fabs(x2 - x1) );
    if ( steep ) {
        std::swap( x1, y1 );
        std::swap( x2, y2 );
    }

    if ( x1 > x2 ) {
        std::swap( x1, x2 );
        std::swap( y1, y2 );
    }

    const float dx = x2 - x1;
    const float dy = fabs( y2 - y1 );

    float error = dx / 2.0f;
    const int ystep = (y1 < y2) ? 1 : -1;
    int y = (int)y1;

    const int maxX = (int)x2;

    for(int x=(int)x1; x<maxX; x++) {
        if(steep) {
            if ( y>=0 && y<_sampling_width && x>=0 && x<_sampling_height ) {
                if ( _pp_map_info[y][x] < OBSTACLE_THRESHOLD ) {
                    return false;
                }
            }
        }
        else {
            if ( x>=0 && x<_sampling_width && y>=0 && y<_sampling_height ) {
                if ( _pp_map_info[x][y] < OBSTACLE_THRESHOLD ) {
                    return false;
                }
            }
        }

        error -= dy;
        if(error < 0) {
            y += ystep;
            error += dx;
        }
    }
    return true;
}

void FMTstar::extend() {
    bool node_inserted = false;
    while( false==node_inserted ) {
        POS2D rnd_pos = _sampling();
        KDNode2D nearest_node = _find_nearest( rnd_pos );

        if (rnd_pos[0]==nearest_node[0] && rnd_pos[1]==nearest_node[1]) {
            continue;
        }

        POS2D new_pos = _steer( rnd_pos, nearest_node );

        if( true == _contains(new_pos) ) {
            continue;
        }
        if( true == _is_in_obstacle( new_pos ) ) {
            continue;
        }

        if( true == _is_obstacle_free( nearest_node, new_pos ) ) {
            std::list<KDNode2D> near_list = _find_near( new_pos );
            KDNode2D new_node( new_pos );

            // create new node
            TreeNode * p_new_rnode = _create_new_node( new_pos );
            new_node.setTreeNode( p_new_rnode );

            _p_kd_tree->insert( new_node );
            node_inserted = true;

            TreeNode* p_nearest_rnode = nearest_node.getTreeNode();
            std::list<TreeNode*> near_rnodes;
            near_rnodes.clear();
            for( std::list<KDNode2D>::iterator itr = near_list.begin();
                itr != near_list.end(); itr++ ) {
                KDNode2D kd_node = (*itr);
                TreeNode* p_near_rnode = kd_node.getTreeNode();
                near_rnodes.push_back( p_near_rnode );
            }

            // attach new node to reference trees
            _attach_new_node( p_new_rnode, p_nearest_rnode, near_rnodes );
            // rewire near nodes of reference trees
            _rewire_near_nodes( p_new_rnode, near_rnodes );
        }
    }
    _current_iteration++;
}

KDNode2D FMTstar::_find_nearest( POS2D pos ) {
    KDNode2D node( pos );

    std::pair<KDTree2D::const_iterator,double> found = _p_kd_tree->find_nearest( node );
    KDNode2D near_node = *found.first;
    return near_node;
}

std::list<KDNode2D> FMTstar::_find_near(POS2D pos) {
    std::list<KDNode2D> near_list;
    KDNode2D node(pos);

    int num_vertices = _p_kd_tree->size();
    int num_dimensions = 2;
    _ball_radius =  _theta * _range * pow( log((double)(num_vertices + 1.0))/((double)(num_vertices + 1.0)), 1.0/((double)num_dimensions) );

    _p_kd_tree->find_within_range( node, _ball_radius, std::back_inserter( near_list ) );

    return near_list;
}


bool FMTstar::_contains( POS2D pos )
{
    if(_p_kd_tree) {
        KDNode2D node( pos[0], pos[1] );
        KDTree2D::const_iterator it = _p_kd_tree->find(node);
        if( it!=_p_kd_tree->end() ) {
            return true;
        }
        else {
            return false;
        }
    }
    return false;
}

double FMTstar::_calculate_cost( POS2D& pos_a, POS2D& pos_b ) {
    return _p_cost_func(pos_a, pos_b, _pp_cost_distribution, this);
}

TreeNode* FMTstar::_create_new_node(POS2D pos) {
    TreeNode * pNode = new TreeNode(pos);
    _nodes.push_back(pNode);

    return pNode;
}

bool FMTstar::_remove_edge(TreeNode* p_node_parent, TreeNode*  p_node_child) {
    if( p_node_parent==NULL ) {
        return false;
    }

    p_node_child->mp_parent = NULL;
    bool removed = false;
    for( std::list<TreeNode*>::iterator it=p_node_parent->m_child_nodes.begin();it!=p_node_parent->m_child_nodes.end();it++ ) {
        TreeNode* p_current = (TreeNode*)(*it);
        if ( p_current == p_node_child || p_current->m_pos==p_node_child->m_pos ) {
            p_current->mp_parent = NULL;
            it = p_node_parent->m_child_nodes.erase(it);
            removed = true;
        }
    }
    return removed;
}

bool FMTstar::_has_edge(TreeNode* p_node_parent, TreeNode* p_node_child) {
    if ( p_node_parent == NULL || p_node_child == NULL ) {
        return false;
    }
    for( std::list<TreeNode*>::iterator it=p_node_parent->m_child_nodes.begin();it!=p_node_parent->m_child_nodes.end();it++ ) {
        TreeNode* p_curr_node = (*it);
        if( p_curr_node == p_node_child ) {
            return true;
        }
    }
    /*
    if (pNode_p == pNode_c->mpParent)
        return true;
    */
    return false;
}

bool FMTstar::_add_edge( TreeNode* p_node_parent, TreeNode* p_node_child ) {
    if( p_node_parent == NULL || p_node_child == NULL || p_node_parent == p_node_child ) {
        return false;
    }
    if ( p_node_parent->m_pos == p_node_child->m_pos ) {
        return false;
    }
    if ( true == _has_edge( p_node_parent, p_node_child ) ) {
        p_node_child->mp_parent = p_node_parent;
    }
    else {
        p_node_parent->m_child_nodes.push_back( p_node_child );
        p_node_child->mp_parent = p_node_parent;
    }
    p_node_child->m_child_nodes.unique();

    return true;
}


std::list<TreeNode*> FMTstar::_find_all_children( TreeNode* p_node ) {
    int level = 0;
    bool finished = false;
    std::list<TreeNode*> child_list;

    std::list<TreeNode*> current_level_nodes;
    current_level_nodes.push_back( p_node );
    while( false==finished ) {
        std::list<TreeNode*> current_level_children;
        int child_list_num = child_list.size();

        for( std::list<TreeNode*>::iterator it=current_level_nodes.begin(); it!=current_level_nodes.end(); it++ ) {
            TreeNode* pCurrentNode = (*it);
            for( std::list<TreeNode*>::iterator itc=pCurrentNode->m_child_nodes.begin(); itc!=pCurrentNode->m_child_nodes.end();itc++ ) {
                TreeNode *p_child_node= (*itc);
                if(p_child_node) {
                    current_level_children.push_back(p_child_node);
                    child_list.push_back(p_child_node);
                }
            }
        }

        child_list.unique();
        current_level_children.unique();

        if (current_level_children.size()==0) {
            finished = true;
        }
        else if (child_list.size()==child_list_num) {
            finished = true;
        }
        else {
            current_level_nodes.clear();
            for( std::list<TreeNode*>::iterator itt=current_level_children.begin();itt!=current_level_children.end();itt++ ) {
                TreeNode * pTempNode = (*itt);
                if( pTempNode ) {
                    current_level_nodes.push_back( pTempNode );
                }
            }
            level +=1;
        }

        if(level>100) {
            break;
        }
    }
    child_list.unique();
    return child_list;
}


TreeNode* FMTstar::_find_ancestor(TreeNode* p_node) {
    return get_ancestor( p_node );
}

Path* FMTstar::find_path() {
    Path* p_new_path = new Path( _start, _goal );

    std::list<TreeNode*> node_list;

    TreeNode * p_first_node = NULL;
    double delta_cost = 0.0;
    _get_closet_to_goal( p_first_node, delta_cost );

    if( p_first_node != NULL ) {
        get_parent_node_list( p_first_node, node_list );
        for( std::list<TreeNode*>::reverse_iterator rit=node_list.rbegin();
            rit!=node_list.rend(); ++rit ) {
            TreeNode* p_node = (*rit);
            p_new_path->m_way_points.push_back( p_node->m_pos );
        }
        p_new_path->m_way_points.push_back(_goal);

        p_new_path->m_cost = p_first_node->m_cost + delta_cost;
    }

    return p_new_path;
}


void FMTstar::_attach_new_node(TreeNode* p_node_new, TreeNode* p_nearest_node, std::list<TreeNode*> near_nodes) {
    double min_new_node_cost = p_nearest_node->m_cost + _calculate_cost(p_nearest_node->m_pos, p_node_new->m_pos);
    TreeNode* p_min_node = p_nearest_node;

    for(std::list<TreeNode*>::iterator it=near_nodes.begin();it!=near_nodes.end();it++) {
        TreeNode* p_near_node = *it;
        if ( true == _is_obstacle_free( p_near_node->m_pos, p_node_new->m_pos ) ) {
            double delta_cost = _calculate_cost( p_near_node->m_pos, p_node_new->m_pos );
            double new_cost = p_near_node->m_cost + delta_cost;
            if ( new_cost < min_new_node_cost ) {
                p_min_node = p_near_node;
                min_new_node_cost = new_cost;
            }
        }
    }

    bool added = _add_edge( p_min_node, p_node_new );
    if( added ) {
        p_node_new->m_cost = min_new_node_cost;
    }

}

void FMTstar::_rewire_near_nodes(TreeNode* p_node_new, std::list<TreeNode*> near_nodes) {
    for( std::list<TreeNode*>::iterator it=near_nodes.begin(); it!=near_nodes.end(); it++ ) {
        TreeNode * p_near_node = (*it);

        if(p_near_node->m_pos ==p_node_new->m_pos ||  p_near_node->m_pos==_p_root->m_pos || p_node_new->mp_parent->m_pos==p_near_node->m_pos) {
            continue;
        }

        if( true == _is_obstacle_free( p_node_new->m_pos, p_near_node->m_pos ) ) {
            double temp_delta_cost = _calculate_cost( p_node_new->m_pos, p_near_node->m_pos );
            double temp_cost_from_new_node = p_node_new->m_cost + temp_delta_cost;
            if( temp_cost_from_new_node < p_near_node->m_cost ) {
                double min_delta_cost = p_near_node->m_cost - temp_cost_from_new_node;
                TreeNode * p_parent_node = p_near_node->mp_parent;
                bool removed = _remove_edge(p_parent_node, p_near_node);
                if(removed) {
                    bool added = _add_edge(p_node_new, p_near_node);
                    if( added ) {
                        p_near_node->m_cost = temp_cost_from_new_node;
                        _update_cost_to_children(p_near_node, min_delta_cost);
                    }
                }
                else {
                    std::cout << " Failed in removing " << std::endl;
                }
            }
        }
    }
}

void FMTstar::_update_cost_to_children( TreeNode* p_node, double delta_cost ) {
    std::list<TreeNode*> child_list = _find_all_children( p_node );
    for( std::list<TreeNode*>::iterator it = child_list.begin(); it != child_list.end();it++ ) {
        TreeNode* p_child_node = (*it);
        if( p_child_node ) {
            p_child_node->m_cost -= delta_cost;
        }
    }
}

bool FMTstar::_get_closet_to_goal( TreeNode*& p_node_closet_to_goal, double& delta_cost ) {
    bool found = false;

    std::list<KDNode2D> near_nodes = _find_near( _goal );
    double min_total_cost = std::numeric_limits<double>::max();

    for(std::list<KDNode2D>::iterator it=near_nodes.begin();
        it!=near_nodes.end();it++) {
        KDNode2D kd_node = (*it);
        TreeNode* p_node = kd_node.getTreeNode();
        double new_delta_cost = _calculate_cost(p_node->m_pos, _goal);
        double new_total_cost= p_node->m_cost + new_delta_cost;
        if (new_total_cost < min_total_cost) {
            min_total_cost = new_total_cost;
            p_node_closet_to_goal = p_node;
            delta_cost = new_delta_cost;
            found = true;
        }
    }
    return found;
}

void FMTstar::dump_distribution(std::string filename) {
    std::ofstream myfile;
    myfile.open (filename.c_str());
    if(_pp_cost_distribution) {
        for(int i=0;i<_sampling_width;i++) {
            for(int j=0;j<_sampling_height;j++) {
                myfile << _pp_cost_distribution[i][j] << " ";
            }
            myfile << "\n";
        }
    }
    myfile.close();
}
