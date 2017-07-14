#include "AthenaClient.h"
#include "protobuf/queryMessage.pb.h"

using namespace athena;
using namespace std;

const string AthenaClient::query(const string &json)
{
    protobufs::queryMessage cmd;
    cmd.set_json(json);

	std::basic_string<uint8_t> msg(cmd.ByteSize(),0);
	cmd.SerializeToArray((void*)msg.data(), msg.length());
	_conn.send_message(msg.data(), msg.length());

    // Wait now for response
    // TODO: Perhaps add an asynchronous version too.
	msg = _conn.recv_message();
	protobufs::queryMessage resp;
	resp.ParseFromArray((const void*)msg.data(), msg.length());

    return resp.json();
}

const string AthenaClient::query(const string &json, const vector<string *> blobs)
{
    protobufs::queryMessage cmd;
    cmd.set_json(json);

    for (auto it : blobs) {
        string *blob = cmd.add_blobs();
        blob = it;
    }
	std::basic_string<uint8_t> msg(cmd.ByteSize(),0);
	cmd.SerializeToArray((void*)msg.data(), msg.length());
	_conn.send_message(msg.data(), msg.length());

    // Wait now for response
    // TODO: Perhaps add an asynchronous version too.
	msg = _conn.recv_message();
	protobufs::queryMessage resp;
	resp.ParseFromArray((const void*)msg.data(), msg.length());

    return resp.json();
}
