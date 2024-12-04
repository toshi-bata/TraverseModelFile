/***********************************************************************************************************************
*
* Copyright (c) 2010 - 2024 by Tech Soft 3D, Inc.
* The information contained herein is confidential and proprietary to Tech Soft 3D, Inc., and considered a trade secret
* as defined under civil and criminal statutes. Tech Soft 3D shall pursue its civil and criminal remedies in the event
* of unauthorized use or misappropriation of its trade secrets. Use of this information by anyone other than authorized
* employees of Tech Soft 3D, Inc. is granted only under a written non-disclosure agreement, expressly prescribing the
* scope and manner of such use.
*
***********************************************************************************************************************/

/*
* This sample demonstrates how to load a model and export it as a model file of a different format. The chosen
* format is determined by the file extension of the output file name.
*/

#define INITIALIZE_A3D_API
#include <A3DSDKIncludes.h>
#include <hoops_license.h>

#include "common.hpp"
#include <sstream>

#include "visitor/VisitorContainer.h"
#include "visitor/VisitorTree.h"
#include "visitor/VisitorCascadedAttribute.h"
#include "visitor/CascadedAttributeConnector.h"
#include "visitor/TransfoConnector.h"
#include "visitor/VisitorTransfo.h"

A3DPtr IAlloc(size_t in_size)
{
	return malloc(in_size);
}
A3DVoid IFree(
	A3DPtr in_ptr)
{
	free(in_ptr);
}

A3DCallbackMemoryAlloc func_alloc = IAlloc;
A3DCallbackMemoryFree func_free = IFree;

static MY_CHAR acSrcFileName[_MAX_PATH * 2];
static MY_CHAR acDstFileName[_MAX_PATH * 2];
static MY_CHAR acLogFileName[_MAX_PATH * 2];

class myTreeVisitor : public A3DTreeVisitor
{
public:
	myTreeVisitor(A3DVisitorContainer* psContainer = NULL) : A3DTreeVisitor(psContainer) {};
	~myTreeVisitor() {};

private:
	int m_iLevel = 0;

public:
	virtual A3DStatus visitEnter(const A3DProductOccurrenceConnector& sConnector) override
	{
		A3DStatus iRet = A3DTreeVisitor::visitEnter(sConnector);

		// Increment level
		m_iLevel++;

		// Get the ProductOccurrence (PO)
		const A3DEntity* pEntity = sConnector.GetA3DEntity();
		A3DAsmProductOccurrence* pPO = (A3DAsmProductOccurrence*)pEntity;

		// Get RootBaseData of the PO
		A3DRootBaseData sRootBaseData;
		A3D_INITIALIZE_DATA(A3DRootBaseData, sRootBaseData);
		A3DRootBaseGet(pPO, &sRootBaseData);

		// Get the PO name  
		A3DUniChar acName[256];
		if (sRootBaseData.m_pcName)
			A3DMiscUTF8ToUTF16(sRootBaseData.m_pcName, acName);
		else
			wcscpy_s(acName, _T("NO_NAME"));

		// Show the PO name with level 
		for (int i = 0; i < m_iLevel; i++)
			_tprintf(_T("+ "));

		_tprintf(_T("%s"), acName);

		A3DVisitorColorMaterials* pA3DCascadedVisitor = static_cast<A3DVisitorColorMaterials*>(m_psContainer->GetVisitorByName("CascadedAttribute"));
		if (pA3DCascadedVisitor)
		{
			ColorMaterialsConnector sColorConnector(nullptr);
			pA3DCascadedVisitor->GetColorMaterialConnector(sColorConnector);

			if (sColorConnector.IsShow())
				_tprintf(_T("\n"));
			else
				_tprintf(_T(" (Hidden)\n"));
		}

		return iRet;
	}

	virtual A3DStatus visitEnter(const A3DPartConnector& sConnector) override
	{
		A3DStatus iRet = A3DTreeVisitor::visitEnter(sConnector);

		// Get transform connector via transform visitor
		A3DVisitorTransfo* psVisitorTransfo = static_cast<A3DVisitorTransfo*>(m_psContainer->GetVisitorByName("Transformation"));
		A3DTransfoConnector* pConnector = psVisitorTransfo->GetTransfoConnector();
		A3DMatrix4x4 sTransfo;
		pConnector->GetGlobalTransfo(sTransfo);
		delete pConnector;

		for (unsigned int i = 0; i < m_iLevel; i++)
			_tprintf(_T("+ "));

		_tprintf(_T(" (%.3f, %.3f, %.3f)\n"), sTransfo.m_adM[12], sTransfo.m_adM[13], sTransfo.m_adM[14]);

		return iRet;
	}

	virtual A3DStatus visitLeave(const A3DProductOccurrenceConnector& sConnector) override
	{
		A3DStatus iRet = A3D_SUCCESS;

		// Decrement level
		m_iLevel--;

		iRet = A3DTreeVisitor::visitLeave(sConnector);
		return iRet;
	}
};

void traverseModelFile(A3DAsmModelFile* pModelFile)
{
	// Prepare Visitor container
	A3DVisitorContainer sA3DVisitorContainer(CONNECT_TRANSFO | CONNECT_COLORS);
	sA3DVisitorContainer.SetTraverseInstance(true);

	// Prepare Tree traverse visitor and set to the container
	A3DTreeVisitor* pMyTreeVisitor = new myTreeVisitor(&sA3DVisitorContainer);
	sA3DVisitorContainer.push(pMyTreeVisitor);

	// Prepare model file connector and call Traverse
	A3DModelFileConnector sModelFileConnector(pModelFile);
	A3DStatus sStatus = sModelFileConnector.Traverse(&sA3DVisitorContainer);

}

//######################################################################################################################
#ifdef _MSC_VER
int wmain(A3DInt32 iArgc, A3DUniChar** ppcArgv)
#else
int main(A3DInt32 iArgc, A3DUTF8Char** ppcArgv)
#endif
{
	//
	// ### COMMAND LINE ARGUMENTS
	//
	if (iArgc > 1) MY_STRCPY(acSrcFileName, ppcArgv[1]);
	else           MY_STRCPY(acSrcFileName, DEFAULT_INPUT_CAD);
	if (iArgc > 2) MY_STRCPY(acDstFileName, ppcArgv[2]);
	else           MY_SPRINTF(acDstFileName, "%s.prc", acSrcFileName);
	if (iArgc > 3) MY_STRCPY(acLogFileName, ppcArgv[3]);
	else           MY_SPRINTF(acLogFileName, "%s_Log.txt", acDstFileName);
	GetLogFile(acLogFileName); // Initialize log file

	//
	// ### INITIALIZE HOOPS EXCHANGE
	//
	std::wstringstream bin_dir;
#ifdef _DEBUG
	std::wstring buffer;
	buffer.resize(_MAX_PATH * 2);
	if (GetEnvironmentVariable(L"HEXCHANGE_INSTALL_DIR", &buffer[0], static_cast<DWORD>(buffer.size())))
	{
		bin_dir << buffer.data() << L"/bin/win64_v142\0";
	}
#else
	bin_dir << L"";

#endif

	A3DSDKHOOPSExchangeLoader sHoopsExchangeLoader(bin_dir.str().data(), HOOPS_LICENSE);
	CHECK_RET(sHoopsExchangeLoader.m_eSDKStatus);

	CHECK_RET(A3DDllSetCallbacksMemory(CheckMalloc, CheckFree));
	CHECK_RET(A3DDllSetCallbacksReport(PrintLogMessage, PrintLogWarning, PrintLogError));

	//
	// ### PROCESS SAMPLE CODE
	//

	// specify input file
	A3DImport sImport(acSrcFileName); // see A3DSDKInternalConvert.hxx for import and export detailed parameters
	
									  // specify output file
	A3DExport sExport(acDstFileName); // see A3DSDKInternalConvert.hxx for import and export detailed parameters

									  // perform conversion
	CHECK_RET(sHoopsExchangeLoader.Import(sImport));

	traverseModelFile(sHoopsExchangeLoader.m_psModelFile);

	CHECK_RET(sHoopsExchangeLoader.Export(sExport));

	//
	// ### TERMINATE HOOPS EXCHANGE
	//

	// Check memory allocations
	return (int)ListLeaks();
}
