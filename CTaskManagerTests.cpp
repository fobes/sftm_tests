#include "pch.h"
#include "../SFTM/CTaskManager.h"

const int nStartTaskCount = 100;
const int nMidTaskCount = 100;
const int nEndTaskCount = 100;
constexpr unsigned long long int nPushedTasks = nStartTaskCount * nMidTaskCount * nEndTaskCount;

std::atomic<unsigned long long int> nExecutedTasks = { 0 };

class CEndTask : public CTask
{
public:
	CEndTask(CTaskCounter& taskCounter) :CTask(taskCounter) {}
	virtual ~CEndTask() {}

public:
	virtual bool Execute(CWorker& worker) noexcept override
	{
		nExecutedTasks++;

		delete this;

		return true;
	}
};
class CMidTask : public CTask
{
public:
	CMidTask(CTaskCounter& taskCounter) :CTask(taskCounter) {}
	virtual ~CMidTask() {}

public:
	virtual bool Execute(CWorker& worker) noexcept override
	{
		CTaskCounter counter;
		for (int n = 0; n < nEndTaskCount; n++)
			EXPECT_TRUE(worker.PushTask(new CEndTask(counter)));
		worker.WorkUntil(counter);

		delete this;

		return true;
	}
};
class CStartTask : public CTask
{
public:
	CStartTask(CTaskCounter& taskCounter) :CTask(taskCounter) {}
	virtual ~CStartTask() {}

public:
	virtual bool Execute(CWorker& worker) noexcept override
	{
		CTaskCounter counter;
		for (int n = 0; n < nMidTaskCount; n++)
			EXPECT_TRUE(worker.PushTask(new CMidTask(counter)));
		worker.WorkUntil(counter);

		delete this;

		return true;
	}
};

TEST(CTaskManager, Starting)
{
	CTaskManager& manager = CTaskManager::GetInstance();

	const auto nWorkersCount = std::thread::hardware_concurrency();
	EXPECT_GT(nWorkersCount, 1);
	EXPECT_TRUE(manager.Start(nWorkersCount));
	EXPECT_EQ(manager.GetWorkersCount(), nWorkersCount);}
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

TEST(CTaskManager, TaskProcessing)
{
	std::cout << "[   INFO   ] Pushed tasks: " << nPushedTasks << std::endl;

	CTaskManager& manager = CTaskManager::GetInstance();
	auto pCurrentWorker = CWorker::GetCurrentThreadWorker();

	CTaskCounter taskCounter;
	for (int n = 0; n < nStartTaskCount; n++)
		ASSERT_TRUE(pCurrentWorker->PushTask(new CStartTask(taskCounter)));
	pCurrentWorker->WorkUntil(taskCounter);

	EXPECT_EQ(nPushedTasks, nExecutedTasks);

	std::cout << "[   INFO   ] Executed tasks: " << nExecutedTasks << std::endl;
}

TEST(CTaskManager, Stopping)
{
	CTaskManager& manager = CTaskManager::GetInstance();
	manager.Stop();
}
