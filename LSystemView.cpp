#include "procgui.h"
#include "LSystemView.h"

namespace procgui
{
    using namespace drawing;
    
    LSystemView::LSystemView(const std::string& name,
                             std::shared_ptr<LSystem> lsys,
                             std::shared_ptr<drawing::InterpretationMap> map,
                             drawing::DrawingParameters params)
        : Observer<LSystem> {lsys}
        , Observer<InterpretationMap> {map}
        , name_ {name}
        , lsys_buff_ {lsys}
        , interpretation_buff_ {map}
        , params_ {params}
        , vertices_ {}
        , bounding_box_ {}
        , sub_boxes_ {}
        , is_selected_ {false}
    {
        // Invariant respected: cohesion between the LSystem/InterpretationMap
        // and the vertices. 
        Observer<LSystem>::add_callback([this](){compute_vertices();});
        Observer<InterpretationMap>::add_callback([this](){compute_vertices();});
        compute_vertices();
    }

    LSystemView::LSystemView(const sf::Vector2f& position)
        : LSystemView(
            "",
            std::make_shared<LSystem>(LSystem("F+F+F+F", {})),
            std::make_shared<drawing::InterpretationMap>(drawing::default_interpretation_map),
            drawing::DrawingParameters({position}))
    {
        // Arbitrary default LSystem.
    }

    LSystemView::LSystemView(const LSystemView& other)
        : Observer<LSystem> {other.Observer<LSystem>::get_target()}
        , Observer<InterpretationMap> {other.Observer<InterpretationMap>::get_target()}
        , name_ {other.name_}
        , lsys_buff_ {other.lsys_buff_}
        , interpretation_buff_ {other.interpretation_buff_}
        , params_ {other.params_}
        , vertices_ {other.vertices_}
        , bounding_box_ {other.bounding_box_}
        , sub_boxes_ {other.sub_boxes_}
        , is_selected_ {other.is_selected_}
    {
        // Manually managing Observer<> callbacks.
        Observer<LSystem>::add_callback([this](){compute_vertices();});
        Observer<InterpretationMap>::add_callback([this](){compute_vertices();});
    }

    LSystemView::LSystemView(LSystemView&& other)
        : Observer<LSystem> {other.Observer<LSystem>::get_target()}
        , Observer<InterpretationMap> {other.Observer<InterpretationMap>::get_target()}
        , name_ {other.name_}
        , lsys_buff_ {std::move(other.lsys_buff_)}
        , interpretation_buff_ {std::move(other.interpretation_buff_)}
        , params_ {other.params_}
        , vertices_ {other.vertices_}
        , bounding_box_ {other.bounding_box_}
        , sub_boxes_ {other.sub_boxes_}
        , is_selected_ {other.is_selected_}
    {
        // Manually managing Observer<> callbacks.
        Observer<LSystem>::add_callback([this](){compute_vertices();});
        Observer<InterpretationMap>::add_callback([this](){compute_vertices();});

        // Remove callbacks of the moved 'other'.
        other.Observer<LSystem>::set_target(nullptr);
        other.Observer<InterpretationMap>::set_target(nullptr);
    }

    LSystemView& LSystemView::operator=(LSystemView other)
    {
        swap(other);
        return *this;
    }

    void LSystemView::swap(LSystemView& other)
    {
        using std::swap;

        // Not a pure swap but Observer<> must be manually swaped:
        //  - swap the targets.
        //  - add correct callback.
        auto tmp_lsys = Observer<LSystem>::get_target();
        Observer<LSystem>::set_target(other.Observer<LSystem>::get_target());
        other.Observer<LSystem>::set_target(tmp_lsys);
    
        Observer<LSystem>::add_callback([this](){compute_vertices();});
        other.Observer<LSystem>::add_callback([&other](){other.compute_vertices();});

        auto tmp_map = Observer<InterpretationMap>::get_target();
        Observer<InterpretationMap>::set_target(other.Observer<InterpretationMap>::get_target());
        other.Observer<InterpretationMap>::set_target(tmp_map);
    
        Observer<InterpretationMap>::add_callback([this](){compute_vertices();});
        other.Observer<InterpretationMap>::add_callback([&other](){other.compute_vertices();});

        swap(name_, other.name_);
        swap(lsys_buff_, other.lsys_buff_);
        swap(interpretation_buff_, other.interpretation_buff_);
        swap(params_, other.params_);
        swap(vertices_, other.vertices_);
        swap(bounding_box_, other.bounding_box_);
        swap(sub_boxes_, other.sub_boxes_);
        swap(is_selected_, other.is_selected_);
    }

    LSystemView LSystemView::clone()
    {        
        // Deep copy.
        return LSystemView(
            name_,
            std::make_shared<LSystem>(*lsys_buff_.get_target()),
            std::make_shared<drawing::InterpretationMap>(*interpretation_buff_.get_target()),
            params_
            );
    }

    LSystemView LSystemView::duplicate()
    {
        return LSystemView(
            name_,
            lsys_buff_.get_target(),
            interpretation_buff_.get_target(),
            params_);
    }

    

    drawing::DrawingParameters& LSystemView::get_parameters()
    {
        return params_;
    }
    LSystemBuffer& LSystemView::get_lsystem_buffer()
    {
        return lsys_buff_;
    }
    InterpretationMapBuffer& LSystemView::get_interpretation_buffer()
    {
        return interpretation_buff_;
    }
    sf::FloatRect LSystemView::get_bounding_box() const
    {
        return bounding_box_;
    }

    
    void LSystemView::compute_vertices()
    {
        // Invariant respected: cohesion between the vertices and the bounding
        // boxes. 
        
        vertices_ = drawing::compute_vertices(*Observer<LSystem>::get_target(),
                                              *Observer<InterpretationMap>::get_target(),
                                              params_);
        bounding_box_ = geometry::compute_bounding_box(vertices_);
        sub_boxes_ = geometry::compute_sub_boxes(vertices_, MAX_SUB_BOXES);
    }
    
    void LSystemView::draw(sf::RenderTarget &target)
    {
        // Interact with the models and re-compute the vertices if there is a
        // modification. 
        if (interact_with(*this, name_, true, &is_selected_))
        {
            compute_vertices();
        }

        // Early out if there are no vertices.
        if (vertices_.size() == 0)
        {
            return;
        }

        // Draw the vertices.
        target.draw(vertices_.data(), vertices_.size(), sf::LineStrip);

        if (is_selected_)
        {
            // Draw the global bounding boxes.
            std::array<sf::Vertex, 5> box =
                {{ {{ bounding_box_.left, bounding_box_.top}},
                   {{ bounding_box_.left, bounding_box_.top + bounding_box_.height}},
                   {{ bounding_box_.left + bounding_box_.width, bounding_box_.top + bounding_box_.height}},
                   {{ bounding_box_.left + bounding_box_.width, bounding_box_.top}},
                   {{ bounding_box_.left, bounding_box_.top}}}};
            target.draw(box.data(), box.size(), sf::LineStrip);
        }

        // DEBUG
        // Draw the sub-bounding boxes.
        // for (const auto& box : sub_boxes_)
        // {
        //     std::array<sf::Vertex, 5> rect =
        //         {{ {{ box.left, box.top}, sf::Color(255,0,0,50)},
        //            {{ box.left, box.top + box.height}, sf::Color(255,0,0,50)},
        //            {{ box.left + box.width, box.top + box.height}, sf::Color(255,0,0,50)},
        //            {{ box.left + box.width, box.top}, sf::Color(255,0,0,50)}}};
        //     target.draw(rect.data(), rect.size(), sf::Quads);
        // }
    }

    bool LSystemView::is_selected() const
    {
        return is_selected_;
    }

    bool LSystemView::is_inside(const sf::Vector2f& click) const
    {
        for (const auto& rect : sub_boxes_)
        {
            if (rect.contains(sf::Vector2f(click)))
            {
                return true;
            }
        }
        return false;
    }
    
    void LSystemView::select()
    {
        is_selected_ = true;
    }
}
