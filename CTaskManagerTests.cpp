#include "pch.h"
#include "../SFTM/CSftmTaskManager.h"

const int nSyncStartTaskCount = 100;
const int nSyncMidTaskCount = 100;
const int nSyncEndTaskCount = 100;
constexpr unsigned long long int nSyncPushedTasks = nSyncStartTaskCount * nSyncMidTaskCount * nSyncEndTaskCount + nSyncMidTaskCount * nSyncEndTaskCount + nSyncEndTaskCount;

constexpr unsigned long long int nAsyncPushedTasks = 10000;

std::atomic<unsigned long long int> nExecutedSyncTasks = { 0 };
std::atomic<unsigned long long int> nExecutedAsyncTasks = { 0 };

class CSyncEndTask : public CSftmTask
{
public:
	CSyncEndTask(CSftmChainController* pChainController) :CSftmTask(pChainController) {}
	virtual ~CSyncEndTask() {}

public:
	virtual bool Execute(CSftmWorker& worker) noexcept override
	{
		nExecutedSyncTasks++;

		delete this;

		return true;
	}
};
class CSyncMidTask : public CSftmTask
{
public:
	CSyncMidTask(CSftmChainController* pChainController) :CSftmTask(pChainController) {}
	virtual ~CSyncMidTask() {}

public:
	virtual bool Execute(CSftmWorker& worker) noexcept override
	{
		for (int n = 0; n < nSyncEndTaskCount; n++)
			EXPECT_TRUE(worker.PushTask(new CSyncEndTask(m_pChainController)));

		nExecutedSyncTasks++;

		delete this;

		return true;
	}
};
class CSyncStartTask : public CSftmTask
{
public:
	CSyncStartTask(CSftmChainController *pChainController) :CSftmTask(pChainController) {}
	virtual ~CSyncStartTask() {}

public:
	virtual bool Execute(CSftmWorker& worker) noexcept override
	{
		for (int n = 0; n < nSyncMidTaskCount; n++)
			EXPECT_TRUE(worker.PushTask(new CSyncMidTask(m_pChainController)));

		nExecutedSyncTasks++;

		delete this;

		return true;
	}
};

class CAsyncEndTask : public CSftmTask
{
public:
	CAsyncEndTask(CSftmChainController* pChainController) :CSftmTask(pChainController) {}
	virtual ~CAsyncEndTask() {}

public:
	virtual bool Execute(CSftmWorker& worker) noexcept override
	{
		nExecutedAsyncTasks++;

		delete this;

		return true;
	}
};

TEST(CSftmTaskManager, Starting)
{
	CSftmTaskManager& manager = CSftmTaskManager::GetInstance();

	const auto nWorkersCount = std::thread::hardware_concurrency();
	EXPECT_GT(nWorkersCount, 1);
	EXPECT_TRUE(manager.Start(nWorkersCount, []() {}));
	EXPECT_EQ(manager.GetWorkersCount(), nWorkersCount);
}
TEST(CSftmTaskManager, GetCurrentWorker)
{
	CSftmTaskManager& manager = CSftmTaskManager::GetInstance();
	auto pCurrentWorker = CSftmWorker::GetCurrentThreadWorker();
	EXPECT_NE(pCurrentWorker, nullptr);
}
TEST(CSftmTaskManager, PushNullTask)
{
	CSftmTaskManager& manager = CSftmTaskManager::GetInstance();
	auto pCurrentWorker = CSftmWorker::GetCurrentThreadWorker();

	EXPECT_FALSE(pCurrentWorker->PushTask(nullptr));
}

TEST(CSftmTaskManager, SyncTaskProcessing)
{
	std::cout << "[   INFO   ] Pushed tasks: " << nSyncPushedTasks << std::endl;

	CSftmTaskManager& manager = CSftmTaskManager::GetInstance();
	auto pCurrentWorker = CSftmWorker::GetCurrentThreadWorker();

	CSftmChainController chainController;
	for (int n = 0; n < nSyncStartTaskCount; n++)
		ASSERT_TRUE(pCurrentWorker->PushTask(new CSyncStartTask(&chainController)));
	pCurrentWorker->WorkUntil(chainController);

	EXPECT_EQ(nSyncPushedTasks, nExecutedSyncTasks);

	std::cout << "[   INFO   ] Executed tasks: " << nExecutedSyncTasks << std::endl;
}
TEST(CSftmTaskManager, AsyncTaskProcessing)
{
	std::cout << "[   INFO   ] Pushed tasks: " << nAsyncPushedTasks << std::endl;

	CSftmTaskManager& manager = CSftmTaskManager::GetInstance();
	auto pCurrentWorker = CSftmWorker::GetCurrentThreadWorker();

	for (int n = 0; n < nAsyncPushedTasks; n++)
		ASSERT_TRUE(pCurrentWorker->PushTask(new CAsyncEndTask(nullptr)));

	while(nAsyncPushedTasks != nExecutedAsyncTasks){}

	std::cout << "[   INFO   ] Executed tasks: " << nExecutedAsyncTasks << std::endl;
}

TEST(CSftmTaskManager, Stopping)
{
	CSftmTaskManager::GetInstance().Stop();
}
