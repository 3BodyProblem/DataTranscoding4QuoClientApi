#ifndef __QLX_UNIT_TEST_H__
#define __QLX_UNIT_TEST_H__
#pragma warning(disable : 4996)
#pragma warning(disable : 4204)


#include <vector>
#include "gtest/gtest.h"


///< --------------------- ��Ԫ�����ඨ�� --------------------------------




///< ------------ ��Ԫ���Գ�ʼ���ඨ�� ------------------------------------


/**
 * @class					UnitTestEnv
 * @brief					ȫ���¼�(��ʼ������)
 * @author					barry
 */
class UnitTestEnv : public testing::Environment
{
public:
	void					SetUp();
	void					TearDown();

private:
};


/**
 * @brief					�������еĲ�������
 * @author					barry
 */
void ExecuteTestCase();



#endif





