#include <stdio.h>
#include <iostream>

#include <vector>
#include<queue>

#include <jsoncpp/json/json.h>
#include <jsoncpp/json/value.h>

#include "node.h"
#include "iterator.h"

class AutoDeleteNode
{
private:
        Json::UInt64 _expiration_timestamp;
        void* _node; //can use void pointer because query only seraches for Nodes and not edges
public:
        AutoDeleteNode(Json::UInt64 new_expiration_timestamp, void* n_node);
        ~AutoDeleteNode();
        Json::UInt64 GetExpirationTimestamp();
        void* GetNode();

};

struct GreaterThanTimestamp
{
    bool operator()(AutoDeleteNode* lhs, AutoDeleteNode* rhs) const
    {
        return lhs->GetExpirationTimestamp() > rhs->GetExpirationTimestamp();
    }
};
