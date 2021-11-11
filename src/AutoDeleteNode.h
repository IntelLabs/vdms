#include <stdio.h>
#include <iostream>

#include <vector>
#include<queue>

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>

#include "node.h"

class AutoDeleteNode
{
private:
        Json::UInt64 _expiration_timestamp;
        PMGD::Node* _node;
public:
        AutoDeleteNode(Json::UInt64 new_expiration_timestamp, PMGD::Node* n_node);
        ~AutoDeleteNode();
        Json::UInt64 GetExpirationTimestamp();
};

struct GreaterThanTimestamp
{
    bool operator()(AutoDeleteNode* lhs, AutoDeleteNode* rhs) const
    {
        return lhs->GetExpirationTimestamp() > rhs->GetExpirationTimestamp();
    }
};
