#pragma once

class ISaveable
{
public:
	virtual void Save() = 0;
	virtual void SaveAs() = 0;
};