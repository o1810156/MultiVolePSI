#pragma once

#include "cryptoTools/Common/Defines.h"
#include "cryptoTools/Common/Matrix.h"
#include "cryptoTools/Common/BitVector.h"
#include "cryptoTools/Common/Timer.h"
#include "cryptoTools/Crypto/PRNG.h"
#include "coproto/coproto.h"

namespace oc = osuCrypto;

namespace experiments
{

	using u64 = oc::u64;
	using u32 = oc::u32;
	using u16 = oc::u16;
	using u8 = oc::u8;

	using oc::ZeroBlock;

	using i64 = oc::i64;
	using i32 = oc::i32;
	using i16 = oc::i16;
	using i8 = oc::i8;

	using block = oc::block;

	template <typename T>
	using span = oc::span<T>;
	template <typename T>
	using Matrix = oc::Matrix<T>;
	template <typename T>
	using MatrixView = oc::MatrixView<T>;

	enum Mode
	{
		Sender = 1,
		Receiver = 2
		// Dual = 3
	};

	struct RequiredBase
	{
		u64 mNumSend;
		oc::BitVector mRecvChoiceBits;
	};

	using PRNG = oc::PRNG;
	using Socket = coproto::Socket;
	using Proto = coproto::task<void>;

	std::unordered_set<block> randomSet(u64 size, u64 seed);
	std::unordered_set<block> randomSetWithCommonItems(u64 size, u64 seed, std::unordered_set<block> commonItems);
	std::unordered_set<block> getIntersection(std::vector<std::unordered_set<block>> sets);
	std::tuple<
		std::vector<std::vector<block>>,
		std::unordered_set<block>>
	createPartySets(u64 nParties, u64 commonSize, u64 setSize, u64 seed);

	enum class SockType
	{
		LocalAsync,
		Asio
	};

	/*
	template <class PSI>
	void test_with_print(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType = SockType::LocalAsync);

	template <class PSI>
	void test(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType = SockType::LocalAsync);

	template <class PSIParty, class PSIDealer>
	void test_with_asio(u64 nParties, u64 commonSize, u64 setSize, u64 seed);
	*/

	template <class PSI>
	void test_with_print(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType);

	template <class PSI>
	void test(u64 nParties, u64 commonSize, u64 setSize, u64 seed, SockType sockType);

	template <class PSIParty, class PSIDealer, class DealerStandaloneRes>
	void test_with_asio(u64 nParties, u64 commonSize, u64 setSize, u64 seed, bool verbose = false);
}