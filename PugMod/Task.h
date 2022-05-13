#pragma once

#define PUG_TASK_EXEC 100
#define PUG_TASK_VOTE 101
#define PUG_TASK_LIST 102
#define PUG_TASK_NEXT 103
#define PUG_TASK_LO3R 104

typedef struct
{
	int   Index;
	float Time;
	float EndTime;
	bool  Loop;
	void* FunctionCallback;
	void*  FunctionParameter;
} P_TASK_INFO, *LP_TASK_INFO;

class CTask
{
public:
	CTask();

	void Clear();
	void Create(int Index, float Time, bool Loop, void* FunctionCallback);
	void Create(int Index, float Time, bool Loop, void* FunctionCallback, void* FunctionParameter);
	bool Exists(int Index);
	void Remove(int Index);
	float Timeleft(int Index);
	void Think();

private:
	std::map<int, P_TASK_INFO> m_Data;
};

extern CTask gTask;
