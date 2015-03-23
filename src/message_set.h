#pragma once

#include <deque>

#include "constants.h"
#include "packet.h"
#include "slice.h"

namespace synkafka {


class MessageSet
{
public:
	struct Message
	{
		slice 			key;
		slice 			value;
		int64_t         offset;
		// Only used internally when reading messages
		CompressionType compression_;
	};

	MessageSet();

	void set_compression(CompressionType comp);

	// Set the max message size as configured with Kafka for this topic
	// You must have your config in sync between Kafka and here - max message size
	// is not reported in topic metadata and may result in messages being reject if you
	// set a higher value. Kafka's defaut is 1000000 bytes so that is our default here too.
	void set_max_message_size(int max_message_size);

	// Push another message onto the set, this will check the message for simple things that
	// will get it rejected. If returned error code is non-zero then the message is NOT added
	// to set and can't be given current configuration.
	// If returned error code has category synkafka and value of synkafka_error::message_set_full
	// then no further pushes will succeed as message bytes would exceed Kafka's max_message_bytes
	// and be rejected.
	// For compressed batches this is potentially sub-optimal but since we can't know until we compressed
	// the whole set how large the compressed message will be, we must be conservative and assume that
	// worst case compression and headers.
	std::error_code push(const slice& message, const slice& key);
	std::error_code push(Message&& m);

	// Allow encode/decode like the primative structs, by the time we get to actually encode
	// it is REQUIRED that the MessageSet is in a valid state (i.e. non empty and not too big).
	// Note friend can't have optional length so this is an implementation function that is called by
	// kafka_proto_io
	friend void kafka_proto_io_impl(PacketCodec& p, MessageSet& ms, int32_t encoded_length);

	const std::deque<Message>& get_messages() const { return messages_; }
	size_t get_encoded_size() const { return encoded_size_; }

private:
	size_t get_msg_encoded_size(const Message& m) const;
	size_t get_worst_case_compressed_size(size_t size) const;

	void message_io(PacketCodec& p, Message& m, CompressionType& compression);

	std::deque<Message> messages_;
	int32_t				max_message_size_;
	CompressionType  	compression_;
	size_t				encoded_size_;
};

void kafka_proto_io(PacketCodec& p, MessageSet::Message& m);

void kafka_proto_io(PacketCodec& p, MessageSet& ms);

/*
inline void kafka_proto_io(PacketCodec& p, Message& m)
{
	// 0.8.x protocol has 0 magic byte
	int8_t magic = 0;

	// Need to encode attributes
	int8_t attributes = 0;

	if (p.is_writer()) {
		// Attributes only contains compression in lowest 2 bits in 0.8.x
		attributes = static_cast<int8_t>(m.compression) & 0x3;
	}

	auto crc = p.start_crc();
	p.io(magic);
	p.io(attributes);

	if (!p.is_writer()) {
		// Attributes only contains compression in lowest 2 bits in 0.8.x
		m.compression = static_cast<CompressionType>(attributes & 0x3);
	}

	p.io_bytes(m.key, COMP_None);
	p.io_bytes(m.value, m.compression);
	p.end_crc(crc);
}


struct MessageSet
{
	CompressionType  						compression;
	std::deque<std::pair<int64_t, Message>> messages;
};

inline 
*/

}