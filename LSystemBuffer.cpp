#include "LSystemBuffer.h"

namespace procgui
{
    LSystemBuffer::LSystemBuffer(const std::shared_ptr<LSystem>& lsys)
        : lsys_ {lsys}
        , buffer_ {}
        , instruction_ {nullptr}
    {
        lsys_::add_callback([this](){sync();});
        
        // Initialize the buffer with the LSystem's rules.
        // By construction, there are not duplicate rules in a 'LSystem', so
        // there is no check: all rules are valid.
        for (const auto& rule : lsys_::target_->get_rules())
        {
            buffer_.push_back({true, rule.first, rule.second});
        }
    }

    bool LSystemBuffer::has_duplicate(const_iterator cit)
    {
        // Find the duplicate of 'cit' in 'buffer_' by comparing the
        // predecessors. 
        return find_duplicate(cit, buffer_.cbegin(), buffer_.cend(),
                              [](const auto& t1, const auto& t2)
                              { return std::get<predecessor>(t1) == std::get<predecessor>(t2); })
            != buffer_.cend();
    }

    LSystemBuffer::const_iterator LSystemBuffer::find_existing(predecessor pred)
    {
        // Find in 'buffer_' an existing rule with the same predecessor as
        // 'pred'. 
        return ::find_existing(buffer_.cbegin(), buffer_.cend(), pred,
                               [](const auto& tuple, const auto& pred)
                               { return std::get<predecessor>(tuple) == pred; });
    }

    LSystem& LSystemBuffer::get_lsys() const
    {
        return *lsys_::target_;
    }

    LSystemBuffer::const_iterator LSystemBuffer::begin() const
    {
        return buffer_.cbegin();
    }

    LSystemBuffer::const_iterator LSystemBuffer::end() const
    {
        return buffer_.cend();
    }

    LSystemBuffer::iterator LSystemBuffer::remove_const(const_iterator cit)
    {
        // https://stackoverflow.com/a/10669041/4309005
        return buffer_.erase(cit, cit);
    }

    
    void LSystemBuffer::add_rule()
    {
        // Add a scratch buffer: a valid empty rule.
        buffer_.push_back({true, '\0', "" });
    }


    void LSystemBuffer::erase(const_iterator cit)
    {
        Expects(cit != buffer_.end());
        
        auto is_valid = std::get<validity>(*cit);
        auto pred = std::get<predecessor>(*cit);

        // If the rule is valid and not a scratch buffer, removes it from the
        // 'LSystem'.
        if (is_valid && pred != '\0')
        {
            lsys_::target_->remove_rule(pred);
        }
        // Otherwise, simply remove it from the buffer.
        else
        {
            buffer_.erase(cit);
            return;
        }
    }

    void LSystemBuffer::change_predecessor(const_iterator cit, predecessor pred)
    {
        Expects(cit != buffer_.end());

        // If the predecessor is null, removes it.
        if (pred == '\0')
        {
            remove_predecessor(cit);
            return;
        }

        auto old_pred = std::get<predecessor>(*cit);
        bool old_has_duplicate = has_duplicate(cit);
        bool duplicate = find_existing(pred) != buffer_.end();
        
        const successor& succ = std::get<successor>(*cit);
        *remove_const(cit) = { !duplicate, pred, succ };

        if (!duplicate)// we added a pred
        {
            lsys_::target_->add_rule(pred, succ);
        }
        if(!old_has_duplicate && old_pred != '\0')
        {
            // auto copy = buffer_.insert(std::next(cit), {true, old_pred, succ});
            // erase(copy);
            lsys_::target_->remove_rule(old_pred);
        }
    }
    
    void LSystemBuffer::remove_predecessor(LSystemBuffer::const_iterator cit)
    {
        Expects(cit != buffer_.end());

        auto old_pred = std::get<predecessor>(*cit);
        auto is_valid = std::get<validity>(*cit);

        auto it = remove_const(cit);
        const successor& succ = std::get<successor>(*cit);
        *it = { true, '\0', succ };

        if (is_valid)
        {
            lsys_::target_->remove_rule(old_pred);
        }
    }


    void LSystemBuffer::change_successor(const_iterator cit, successor succ)
    {
        Expects(cit != buffer_.end());

        auto it = remove_const(cit);

        auto valid = std::get<validity>(*cit);
        auto pred = std::get<predecessor>(*cit);
        *it = { valid, pred, succ };

        if (valid)
        {
            lsys_::target_->add_rule(pred, succ);
        }
    }
    
    void LSystemBuffer::delayed_add_rule()
    {
        instruction_ = [=](){ add_rule(); };
    }
    void LSystemBuffer::delayed_erase(const_iterator cit)
    {
        instruction_ = [=](){ erase(cit); };
    }
    void LSystemBuffer::delayed_change_predecessor(const_iterator cit, predecessor pred)
    {
        instruction_ = [=](){ change_predecessor(cit, pred); };
    }
    void LSystemBuffer::delayed_remove_predecessor(const_iterator cit)
    {
        instruction_ = [=](){ remove_predecessor(cit); };
    }
    void LSystemBuffer::delayed_change_successor(const_iterator cit, successor succ)
    {
        instruction_ = [=](){ change_successor(cit, succ); };
    }

    void LSystemBuffer::apply()
    {
        if(instruction_)
        {
            instruction_();
            instruction_ = nullptr;
        }
    }

    void LSystemBuffer::sync()
    {
        const auto& lsys_rules = lsys_::target_->get_rules();

        // gérer les ajouts
        // gérer les suppressions
        // gérer les modifications
        // gérer les \0
        // gérer les non valides

        for (const auto& rule : lsys_rules)
        {
            char pred = rule.first;
            auto it = std::find_if(buffer_.begin(), buffer_.end(),
                                   [pred](const auto& tuple)
                                   { return std::get<predecessor>(tuple) == pred &&
                                     std::get<validity>(tuple); });
            if (it != buffer_.end())
            {
                std::get<successor>(*it) = rule.second;
            }
            else
            {
                buffer_.push_back({true, rule.first, rule.second});
            }
        }

        for (auto it = buffer_.begin(); it != buffer_.end(); )
        {
            auto valid = std::get<validity>(*it);
            auto pred = std::get<predecessor>(*it);

            if(valid && pred != '\0')
            {
                bool exist = lsys_rules.count(pred) > 0 ? true : false;
                if (!exist)
                {
                    it = buffer_.erase(it);
                }
                else
                {
                    ++it;
                }
            }
            else
            {
                ++it;
            }
        }

        for (auto it = buffer_.begin(); it != buffer_.end(); ++it)
        {
            if (!std::get<validity>(*it))
            {
                bool duplicate = false;
                for (auto jt = buffer_.begin(); jt != buffer_.end(); ++jt)
                {
                    if (it != jt &&
                        std::get<predecessor>(*it) == std::get<predecessor>(*jt))
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                {
                    std::get<validity>(*it) = true;
                    lsys_::target_->add_rule(std::get<predecessor>(*it), std::get<successor>(*it));
                }
            }
        }
    }
}