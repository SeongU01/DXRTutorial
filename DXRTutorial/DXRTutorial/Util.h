#pragma once
#define FAILED_CHECK_BREAK(hr) if (FAILED(hr)) __debugbreak()

template <typename T>
class SingleTon
{
protected:
	SingleTon() = default;
	~SingleTon() = default;
public:
	static T* GetInstance()
	{
		if (nullptr == _pInstance)
			_pInstance = new T;
		return _pInstance;
	}
	static void ResetInstance() { _pInstance = nullptr; }
private:
	static T* _pInstance;
};
template <typename T>
T* SingleTon<T>::_pInstance = nullptr;

