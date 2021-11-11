#include "AutoDeleteNode.h"

AutoDeleteNode::AutoDeleteNode(Json::UInt64 new_expiration_timestamp, PMGD::Node* new_node)
{
  _expiration_timestamp = new_expiration_timestamp;
  _node = new_node;
}

AutoDeleteNode::~AutoDeleteNode()
{
}

Json::UInt64 AutoDeleteNode::GetExpirationTimestamp()
{
  return _expiration_timestamp;
}
