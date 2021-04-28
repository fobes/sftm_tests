#include "pch.h"
#include "../SFTM/CTaskManager.h"

const int nSyncStartTaskCount = 100;
const int nSyncMidTaskCount = 100;
const int nSyncEndTaskCount = 100;
constexpr unsigned long long int nSyncPushedTasks = nSyncStartTaskCount * nSyncMidTaskCount * nSyncEndTaskCount + nSyncMidTaskCount * nSyncEndTaskCount + nSyncEndTaskCount;

constexpr unsigned long long int nAsyncPushedTasks = 10000;

std::atomic<unsigned long long int> nExecutedSyncTasks = { 0 };
std::atomic<unsigned long long int> nExecutedAsyncTasks = { 0 };

class CSyncEndTask : public CTask
{
public:
	CSyncEndTask(CChainController* pChainController) :CTask(pChainController) {}
	virtual ~CSyncEndTask() {}

public:
	virtual bool Execute(CWorker& worker) noexcept override
	{
		nExecutedSyncTasks++;

		delete this;

		return true;
	}
};
class CSyncMidTask : public CTask
{
public:
	CSyncMidTask(CChainController* pChainController) :CTask(pChainController) {}
	virtual ~CSyncMidTask() {}

public:
	virtual bool Execute(CWorker& worker) noexcept override
	{
		for (int n = 0; n < nSyncEndTaskCount; n++)
			EXPECT_TRUE(worker.PushTask(new CSyncEndTask(m_pChainController)));

		nExecutedSyncTasks++;

		delete this;

		return true;
	}
};
class CSyncStartTask : public CTask
{
public:
	CSyncStartTask(CChainController *pChainController) :CTask(pChainController) {}
	virtual ~CSyncStartTask() {}

public:
	virtual bool Execute(CWorker& worker) noexcept override
	{
		for (int n = 0; n < nSyncMidTaskCount; n++)
			EXPECT_TRUE(worker.PushTask(new CSyncMidTask(m_pChainController)));

		nExecutedSyncTasks++;

		delete this;

		return true;
	}
};

class CAsyncEndTask : public CTask
{
public:
	CAsyncEndTask(CChainController* pChainController) :CTask(pChainController) {}
	virtual ~CAsyncEndTask() {}

public:
	virtual bool Execute(CWorker& worker) noexcept override
	{
		nExecutedAsyncTasks++;

		delete this;

		return true;
	}
};

TEST(CTaskManager, Starting)
{
	CTaskManager& manager = CTaskManager::GetInstance();

	const auto nWorkersCount = std::thread::hardware_concurrency();
	EXPECT_GT(nWorkersCount, 1);
	EXPECT_TRUE(manager.Start(nWorkersCount, []() {}));
	EXPECT_EQ(manager.GetWorkersCount(), nWorkersCount);
}
TEST(CTaskManager, GetCurrentWorker)
{
	CTaskManager& manager = CTaskManager::GetInstance();
	auto pCurrentWorker = CWorker::GetCurrentThreadWorker();
	EXPECT_NE(pCurrentWorker, nullptr);
}
TEST(CTaskManager, PushNullTask)
{
	CTaskManager& manager = CTaskManager::GetInstance();
	auto pCurrentWorker = CWorker::GetCurrentThreadWorker();

	EXPECT_FALSE(pCurrentWorker->PushTask(nullptr));
}

TEST(CTaskManager, SyncTaskProcessing)
{
	std::cout << "[   INFO   ] Pushed tasks: " << nSyncPushedTasks << std::endl;

	CTaskManager& manager = CTaskManager::GetInstance();
	auto pCurrentWorker = CWorker::GetCurrentThreadWorker();

	CChainController chainController;
	for (int n = 0; n < nSyncStartTaskCount; n++)
		ASSERT_TRUE(pCurrentWorker->PushTask(new CSyncStartTask(&chainController)));
	pCurrentWorker->WorkUntil(chainController);

	EXPECT_EQ(nSyncPushedTasks, nExecutedSyncTasks);

	std::cout << "[   INFO   ] Executed tasks: " << nExecutedSyncTasks << std::endl;
}
TEST(CTaskManager, AsyncTaskProcessing)
{
	std::cout << "[   INFO   ] Pushed tasks: " << nAsyncPushedTasks << std::endl;

	CTaskManager& manager = CTaskManager::GetInstance();
	auto pCurrentWorker = CWorker::GetCurrentThreadWorker();

	for (int n = 0; n < nAsyncPushedTasks; n++)
		ASSERT_TRUE(pCurrentWorker->PushTask(new CAsyncEndTask(nullptr)));

	while(nAsyncPushedTasks != nExecutedAsyncTasks){}

	std::cout << "[   INFO   ] Executed tasks: " << nExecutedAsyncTasks << std::endl;
}

TEST(CTaskManager, Stopping)
{
	CTaskManager::GetInstance().Stop();
}
