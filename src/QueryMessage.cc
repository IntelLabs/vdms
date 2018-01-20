
#include "QueryMessage.h"
#include "Exception.h"

using namespace athena;

QueryMessage::QueryMessage(comm::Connection* conn):
		_conn(conn)
{
	if (_conn == NULL)
		throw ExceptionServer(NullConnection);
}

protobufs::queryMessage QueryMessage::get_query()
{
	const std::basic_string<uint8_t>& msg = _conn->recv_message();

	protobufs::queryMessage cmd;
	cmd.ParseFromArray((const void*)msg.data(), msg.length());

	return cmd;
}

void QueryMessage::send_response(protobufs::queryMessage cmd)
{
	std::basic_string<uint8_t> msg(cmd.ByteSize(),0);
	cmd.SerializeToArray((void*)msg.data(), msg.length());
	_conn->send_message(msg.data(), msg.length());
}
