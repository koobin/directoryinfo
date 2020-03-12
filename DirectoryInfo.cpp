
//#include "StdAfx.h"

#include <assert.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <ShlObj.h>

//#include "GenericHelper.h"
#include "DirectoryInfo.h"

#pragma comment(lib, "shlwapi.lib")

// #### FileTimeToTime_t ####
// void  FileTimeToTime_t(FILETIME  ft, time_t  *t)
// {
// 	//LONGLONG  ll;  
// 
// 	ULARGE_INTEGER            ui;
// 	ui.LowPart = ft.dwLowDateTime;
// 	ui.HighPart = ft.dwHighDateTime;
// 
// 	//ll            =  (LONGLONG(ft.dwHighDateTime)<<32)  +  ft.dwLowDateTime;  
// 
// 	*t = ((LONGLONG)(ui.QuadPart - 116444736000000000) / 10000000);
// }


// DirectoryInfo::DirectoryInfo()
// 	: m_nFiles(0)
// 	, m_nDirs(0)
// 	, m_llSize(0)
// 	, m_llSpace(0)
// 	, m_bRecursion(TRUE)
// {
// 	ZeroMemory(m_szPattern, sizeof(m_szPattern));
// 	lstrcpy(m_szPattern, _T("*"));
// 
// 	Proceed(path);
// }
//
// DirectoryInfo::DirectoryInfo(LPCTSTR path, LPCTSTR searchPatten)
// 	: m_nFiles(0)
// 	, m_nDirs(0)
// 	, m_llSize(0)
// 	, m_llSpace(0)
// 	, m_bRecursion(TRUE)
// {
// 	ZeroMemory(m_szPattern, sizeof(m_szPattern));
// 	lstrcpy(m_szPattern, searchPatten);
// 
// 	Proceed(path);
// }

DirectoryInfo::DirectoryInfo(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion, DirectoryCallback func_cb)
	: m_nFiles(0)
	, m_nDirs(0)
	, m_llSize(0)
	, m_llSpace(0)
	, m_cbTraversal(std::move(func_cb))
{
	PathWrapper fpath = path;
	fpath.Absolute();

	Proceed(fpath, searchPatten, recursion);
}

BOOL DirectoryInfo::Proceed(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion)
{
	PathWrapper pathr(path);
	pathr.StripToRoot();

	if (pathr == path) // 根目录
	{
		UINT64 i64FreeBytesToCaller, i64TotalBytes, i64FreeBytes;

		BOOL fResult = GetDiskFreeSpaceEx(path,
			(PULARGE_INTEGER)&i64FreeBytesToCaller,
			(PULARGE_INTEGER)&i64TotalBytes,
			(PULARGE_INTEGER)&i64FreeBytes);

		if (fResult)
		{
			filesize = i64TotalBytes-i64FreeBytes;
			filespace = i64FreeBytes;
		}
		else
			return FALSE;
	}
	else
	{
		WIN32_FIND_DATA FindData;
		HANDLE hFind;

		hFind = FindFirstFile(path, &FindData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			_tprintf(_T("error: %s not found!\n"), (LPCTSTR)path);
			return FALSE;
		}
		FindClose(hFind);

		createTime = FileOper::FileTimeToTime_t(FindData.ftCreationTime);
		lastAccessTime = FileOper::FileTimeToTime_t(FindData.ftLastAccessTime);
		lastWriteTime = FileOper::FileTimeToTime_t(FindData.ftLastWriteTime);
		fileAttributes = FindData.dwFileAttributes;
		filesize = (UINT64(FindData.nFileSizeHigh) << 32) + FindData.nFileSizeLow;

	}
	lstrcpy(filepath, path);

	Traversal(path, searchPatten, recursion);

	return TRUE;
}

DirectoryInfo::~DirectoryInfo(void)
{
	files.clear();
}

void DirectoryInfo::Refresh(LPCTSTR searchPatten, BOOL recursion, DirectoryCallback func_cb)
{
	m_nFiles=0; m_nDirs = 0; 
	m_llSize = 0; m_llSpace = 0;
	m_cbTraversal = std::move(func_cb);
	files.clear();

	Proceed(filepath, searchPatten, recursion);
}

BOOL DirectoryInfo::Delete(BOOL recursive)
{
	return DirectoryOper::Delete(filepath, recursive);
}

BOOL DirectoryInfo::MoveTo(LPCTSTR destDir, BOOL overwrite)
{
	return DirectoryOper::Move(filepath, destDir, overwrite);
}

BOOL DirectoryInfo::CreateSubdir(LPCTSTR destDirName)
{
	PathWrapper path(filepath);
	path.Append(destDirName);
	return DirectoryOper::Create(path, TRUE);
}

BOOL DirectoryInfo::IsExists() const
{
	return ::PathFileExists(filepath);
}

// 深度优先递归遍历目录中所有的文件
BOOL DirectoryInfo::Traversal(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion)
{
	WIN32_FIND_DATA FindData;
	HANDLE hFind;

	// 构造路径
// 	TCHAR FilePathName[MAX_PATH] = { 0 };
// 	lstrcpy(FilePathName, path);
// 	::PathAppend(FilePathName, m_szPattern);
	PathWrapper FilePathName(path);
	FilePathName.Append(searchPatten);

	hFind = FindFirstFile(FilePathName, &FindData);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		_tprintf(_T("error: %s<DIR> + %s<Patten> not found!\n"), path, searchPatten);
		return FALSE;
	}
	while (::FindNextFile(hFind, &FindData))
	{
		// 过虑 . 和 ..
		if (lstrcmp(FindData.cFileName, _T(".")) == 0 ||
			lstrcmp(FindData.cFileName, _T("..")) == 0)
		{
			continue;
		}

		// 输出本级的文件
		//oper(&FindData, path, lpParam);  // 前序遍历

		if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			m_nDirs++;
			if (recursion)
			{
				// 构造完整路径
// 				TCHAR FullPathName[MAX_PATH];
// 				lstrcpy(FullPathName, path);
// 				::PathAppend(FullPathName, FindData.cFileName);
				PathWrapper FullPathName(path);
				FullPathName.Append(FindData.cFileName);
				Traversal(FullPathName, searchPatten, recursion);
			}
		}
		else
		{
			m_nFiles++;
			UINT64 size = (UINT64(FindData.nFileSizeHigh)<<32)+FindData.nFileSizeLow;
			m_llSize += size;
			m_llSpace += (size + 4095) / 4096 * 4096;
		}

		// 输出本级的文件
		OperFindData(&FindData, path);  // 后序遍历
		//Run(&FindData, path);
	}

	FindClose(hFind);

	return TRUE;
}

void DirectoryInfo::OperFindData(const WIN32_FIND_DATA* finddata, LPCTSTR path)
{
	TCHAR FullPathName[MAX_PATH] = { 0 };
	lstrcpy(FullPathName, path);
	::PathAppend(FullPathName, finddata->cFileName);

	filesysinfo_ptr file = filesysinfo_ptr(new TFileSysInfo());
	file->lastWriteTime = FileOper::FileTimeToTime_t(finddata->ftLastWriteTime);
	file->lastAccessTime = FileOper::FileTimeToTime_t(finddata->ftLastAccessTime);
	file->createTime = FileOper::FileTimeToTime_t(finddata->ftCreationTime);
	file->fileAttributes = finddata->dwFileAttributes;
	file->filesize = (UINT64(finddata->nFileSizeHigh) << 32) + finddata->nFileSizeLow;
	file->filespace = (file->filesize + 4095) / 4096 * 4096;
	lstrcpy(file->filepath, FullPathName);

	files.push_back(file);

	if (m_cbTraversal)  m_cbTraversal(finddata, path);
}

// void DirectoryInfo::Reset()
// {
// 	__super::Reset();
// 	  m_nFiles=0; m_nDirs = 0; 
// 	  m_llSize = 0; m_llSpace = 0; 
// 	  m_bRecursion = TRUE;
// 	  ZeroMemory(m_szPattern, sizeof(m_szPattern));
// // 		while (files.size() > 0)
// // 		{
// // 			fileinfo_ptr file = files.back();
// // 			files.pop_back();
// // 			delete file;
// // 		}
// 	  files.clear();
// }

LPCTSTR DirectoryInfo::GetParent() const
{
	PathWrapper path(filepath);
	//path.RemoveBackslash();
	path.RemoveFileSpec();
	BOOL isRoot = (path==filepath);
	if (!isRoot) lstrcpy((LPTSTR)m_szTemp, (LPCTSTR)path);
	return (isRoot ? nullptr : m_szTemp);
}

LPCTSTR DirectoryInfo::GetRoot() const
{
	PathWrapper path(filepath);
	path.StripToRoot();
	lstrcpy((LPTSTR)m_szTemp, (LPCTSTR)path);
	return m_szTemp;
}

filesysinfos_t DirectoryInfo::GetFileSysInfos(EnFilesType type /*= FT_ALL*/) const
{
	filesysinfos_t outfiles;
	switch (type)
	{
	case FT_ALL:
		outfiles = files;
		break;
	case FT_FILE:
		outfiles.clear();
		for each(auto& file in files) {
			if ((file->fileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0){
				outfiles.push_back(file);
			}
		}
		break;
	case FT_DIR:
		outfiles.clear();
		for each(auto& file in files) {
			if (file->fileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				outfiles.push_back(file);
			}
		}
		break;
	default:
		assert(false);
		return (outfiles);
	}
	return (outfiles);
}

// Create a new directory. 
BOOL DirectoryOper::Create(LPCTSTR path, BOOL recursion)
{
	if (::PathIsDirectory(path)) 	
		return TRUE;

	if (recursion)
	{
		int ret = ::SHCreateDirectoryEx(NULL, path, NULL)	;
		if (ret != ERROR_SUCCESS && ret!=ERROR_ALREADY_EXISTS && ret!=ERROR_FILE_EXISTS)
		{
			//ERROR_SUCCESS || ERROR_ALREADY_EXISTS ||	ERROR_FILE_EXISTS
			//TRACE1("SHCreateDirectory failed! error code=%d.\n", ret);
			return FALSE;
		}
		return TRUE;
	} 
	else
	{
		if (!::CreateDirectory(path, NULL)) 
		{ 
			int ret = ::GetLastError();
			if (ret != ERROR_ALREADY_EXISTS)
			{
				//ERROR_SUCCESS || ERROR_ALREADY_EXISTS ||	ERROR_FILE_EXISTS
				//TRACE1("SHCreateDirectory failed! error code=%d.\n", ret);
				return FALSE;
			}
		}
		return TRUE;
	}
}

BOOL DirectoryOper::Delete(LPCTSTR path, BOOL recursion)
{
	// 确保path尾部两个0结尾，使得SHFileOperation函数正确执行
	TCHAR FullPathName[MAX_PATH] = { 0 };
	lstrcpy(FullPathName, path);

	if (recursion)
	{
		SHFILEOPSTRUCT FileOp;
		ZeroMemory((void*)&FileOp, sizeof(SHFILEOPSTRUCT));

		FileOp.fFlags = FOF_NO_UI;
		FileOp.pFrom = FullPathName;
		FileOp.wFunc = FO_DELETE;

		int ret =::SHFileOperation(&FileOp);
		return (ret == 0);
	} 
	else
	{
		return ::RemoveDirectory(FullPathName);
	}
}

BOOL DirectoryOper::Copy(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite)
{
	PathWrapper destpath(destPath);
	if (PathIsDirectory(destpath) && overwrite==FALSE) 	
		return FALSE;

	// 确保path尾部两个0结尾，使得SHFileOperation函数正确执行
	TCHAR FullPathName[MAX_PATH] = { 0 };
	lstrcpy(FullPathName, srcPath);

	SHFILEOPSTRUCT FileOp;
	ZeroMemory((void*)&FileOp,sizeof(SHFILEOPSTRUCT));
	FileOp.fFlags = FOF_NO_UI ;
	FileOp.hNameMappings = NULL;
	FileOp.hwnd = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.pFrom = FullPathName;
	FileOp.pTo = destpath;
	FileOp.wFunc = FO_COPY;

	return (SHFileOperation(&FileOp) == 0);
}

BOOL DirectoryOper::Move(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite)
{
// 	PathWrapper srcpath(srcPath), destpath(destPath);
// 	srcpath.RemoveBackslash();
// 	srcpath.StripPath();
// 	destpath.Append(srcpath);

	PathWrapper destpath(destPath);
	if (PathIsDirectory(destpath) && overwrite==FALSE) 	
		return FALSE;

	// 确保path尾部两个0结尾，使得SHFileOperation函数正确执行
	TCHAR FullPathName[MAX_PATH] = { 0 };
	lstrcpy(FullPathName, srcPath);

	SHFILEOPSTRUCT FileOp;
	ZeroMemory((void*)&FileOp,sizeof(SHFILEOPSTRUCT));
	FileOp.fFlags = FOF_NO_UI ;
	FileOp.hNameMappings = NULL;
	FileOp.hwnd = NULL;
	FileOp.lpszProgressTitle = NULL;
	FileOp.pFrom = FullPathName;
	FileOp.pTo = destpath;
	FileOp.wFunc = FO_MOVE;

	return (SHFileOperation(&FileOp) == 0);
}

BOOL DirectoryOper::Rename(LPCTSTR srcPath, LPCTSTR newName)
{
	PathWrapper path(newName);
	if (path.IsFileSpec())
	{
		path = srcPath;
		path.RemoveBackslash();
		path.RemoveFileSpec();
		path.Append(newName);
		return Move(srcPath, path, TRUE);
	}
	else
		return FALSE;
}

BOOL DirectoryOper::IsExists(LPCTSTR path)
{
	return ::PathFileExists(path);
}

filesysinfos_t DirectoryOper::GetFiles(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion)
{
	DirectoryInfo dirinfo(path, searchPatten, recursion);
	return dirinfo.GetFileSysInfos(DirectoryInfo::FT_FILE);
}

filesysinfos_t DirectoryOper::GetDirectories(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion)
{
	DirectoryInfo dirinfo(path, searchPatten, recursion);
	return dirinfo.GetFileSysInfos(DirectoryInfo::FT_DIR);
}

filesysinfos_t DirectoryOper::GetFileSysInfos(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion)
{
	DirectoryInfo dirinfo(path, searchPatten, recursion);
	return dirinfo.GetFileSysInfos(DirectoryInfo::FT_ALL);
}

BOOL DirectoryOper::GetCurrent(LPTSTR lpBuffer, DWORD nBufferLength)
{
	DWORD dwRet = ::GetCurrentDirectory(nBufferLength, lpBuffer);
	return (( dwRet == 0 || dwRet > nBufferLength) ? FALSE : TRUE);
}

PathWrapperPtr DirectoryOper::GetCurrent()
{
	PathWrapperPtr path(new PathWrapper());
	DWORD len = ::GetCurrentDirectory(_MAX_PATH, path->m_szPath);
	path->AddBackslash();

	return path;
}

BOOL DirectoryOper::SetCurrent(LPCTSTR lpPathName)
{
	return ::SetCurrentDirectory(lpPathName);
}

PathWrapperPtr DirectoryOper::GetModule()
{
	PathWrapperPtr path(new PathWrapper());

	if (::GetModuleFileName(NULL, path->m_szPath, MAX_PATH - 1))
		path->RemoveFileSpec();
	path->AddBackslash();

	return path;
}

BOOL DirectoryOper::GetModule(LPTSTR lpBuffer, DWORD nBufferLength)
{
	DWORD dwRet = ::GetModuleFileName(NULL, lpBuffer, nBufferLength);
	if( dwRet == 0 || dwRet > nBufferLength)
	{
		return FALSE;
	}
	::PathRemoveFileSpec(lpBuffer);
//	::PathAddBackslash(lpBuffer);

	return TRUE;
}

PathWrapper::PathWrapper()
{
	ZeroMemory(m_szPath, sizeof(m_szPath));
	//Canonicalize();
}

PathWrapper::PathWrapper(LPCTSTR path)
{
	ZeroMemory(m_szPath, sizeof(m_szPath));
	lstrcpy(m_szPath, path);
}

PathWrapper& PathWrapper::operator=(LPCTSTR path)
{
	ZeroMemory(m_szPath, sizeof(m_szPath));
	lstrcpy(m_szPath, path);

	return *this;
}

BOOL PathWrapper::operator==(const PathWrapper& path)
{
	return (lstrcmpi(m_szPath, path.Get())==0);
}

BOOL PathWrapper::operator==(LPCTSTR path)
{
	return (lstrcmpi(m_szPath, path)==0);
}

LPCTSTR PathWrapper::AddBackslash()
{
	return ::PathAddBackslash(m_szPath);
}

BOOL PathWrapper::AddExtension(LPCTSTR pszExtension) 
{
	return ::PathAddExtension(m_szPath, pszExtension); 
}

BOOL PathWrapper::Append( LPCTSTR pszMore) 
{
	return ::PathAppend(m_szPath, pszMore); 
}

BOOL PathWrapper::Canonicalize()
{ 
	TCHAR temp[MAX_PATH] = { 0 };
	lstrcpy(temp, m_szPath);
	return ::PathCanonicalize(m_szPath, temp);
}

void PathWrapper::Absolute()
{
	if (IsRelative())
	{
		auto curpath = DirectoryOper::GetCurrent();
		curpath->Append(m_szPath);
		*this = *curpath;
	}
	Append(_T("."));  // 规整尾部'/'的情况
}

int PathWrapper::CommonPrefix(LPCTSTR pszFile, LPTSTR pszPath)
{ 
	return ::PathCommonPrefix(m_szPath, pszFile, pszPath);
}

BOOL PathWrapper::CompactPath(LPTSTR pszOut, UINT cchMax)
{ 
	return ::PathCompactPathEx(pszOut, m_szPath, cchMax, 0); 
}

BOOL PathWrapper::IsExists()
{ 
	return ::PathFileExists(m_szPath);
}

LPCTSTR PathWrapper::FindExtension() 
{ 
	return ::PathFindExtension(m_szPath); 
}

LPCTSTR PathWrapper::FindFileName() 
{
	return ::PathFindFileName(m_szPath); 
}

LPCTSTR PathWrapper::FindNextComponent()
{ 
	return ::PathFindNextComponent(m_szPath); 
}

BOOL PathWrapper::FindOnPath(LPTSTR pszFile) 
{ 
	LPCTSTR pszPath = m_szPath; 
	return ::PathFindOnPath(pszFile, &pszPath); 
}

LPCTSTR PathWrapper::FindSuffixArray(LPCTSTR *apszSuffix, int iArraySize)
{
	return ::PathFindSuffixArray(m_szPath, apszSuffix, iArraySize); 
}

LPCTSTR PathWrapper::GetArgs() 
{
	return ::PathGetArgs(m_szPath);
}

int PathWrapper::GetDriveNumber()
{
	return ::PathGetDriveNumber(m_szPath); 
}

BOOL PathWrapper::IsContentType(LPCTSTR pszContentType)
{
	return ::PathIsContentType(m_szPath, pszContentType); 
}

BOOL PathWrapper::IsDirectory() 
{
	return ::PathIsDirectory(m_szPath); 
}

BOOL PathWrapper::IsDirectoryEmpty()
{
	return ::PathIsDirectoryEmpty(m_szPath);
}

BOOL PathWrapper::IsFileSpec()
{
	return ::PathIsFileSpec(m_szPath); 
}

BOOL PathWrapper::IsNetworkPath()
{ 
	return ::PathIsNetworkPath(m_szPath); 
}

BOOL PathWrapper::IsPrefix(IN LPCTSTR pszPrefix)
{
	return ::PathIsPrefix(m_szPath, pszPrefix); 
}

BOOL PathWrapper::IsRelative()
{
	return ::PathIsRelative(m_szPath);
}

BOOL PathWrapper::IsRoot()
{
	return ::PathIsRoot(m_szPath);
}

BOOL PathWrapper::IsSameRoot(LPCTSTR pszPath)
{
	return ::PathIsSameRoot(m_szPath, pszPath);
}

BOOL PathWrapper::IsSystemFolder()
{
	return ::PathIsSystemFolder(m_szPath, 0);
}

BOOL PathWrapper::IsUNC()
{
	return ::PathIsUNC(m_szPath);
}

BOOL PathWrapper::IsURL()
{
	return ::PathIsURL(m_szPath);
}

BOOL PathWrapper::MakePretty()
{
	return ::PathMakePretty(m_szPath);
}

BOOL PathWrapper::MakeSystemFolder()
{
	return ::PathMakeSystemFolder(m_szPath);
}

BOOL PathWrapper::MatchSpec(LPCTSTR pszSpec)
{
	return ::PathMatchSpec(m_szPath, pszSpec);
}

int	 PathWrapper::ParseIconLocation()
{
	return ::PathParseIconLocation(m_szPath);
}

void PathWrapper::QuoteSpaces()
{
	::PathQuoteSpaces(m_szPath);
}

void PathWrapper::RemoveArgs()
{
	::PathRemoveArgs(m_szPath);
}

LPCTSTR PathWrapper::RemoveBackslash()
{
	return ::PathRemoveBackslash(m_szPath);
}

void   PathWrapper::RemoveBlanks()
{
	::PathRemoveBlanks(m_szPath);
}

void   PathWrapper::RemoveExtension()
{
	::PathRemoveExtension(m_szPath);
}

BOOL PathWrapper::RemoveFileSpec()
{
	return ::PathRemoveFileSpec(m_szPath);
}

BOOL PathWrapper::RenameExtension(LPCTSTR pszExt)
{
	return ::PathRenameExtension(m_szPath, pszExt);
}

LPCTSTR PathWrapper::SkipRoot()
{
	return ::PathSkipRoot(m_szPath);
}

void   PathWrapper::StripPath()
{
	::PathStripPath(m_szPath);
}

BOOL PathWrapper::StripToRoot()
{
	return ::PathStripToRoot(m_szPath);
}

void   PathWrapper::Undecorate()
{
	::PathUndecorate(m_szPath);
}

BOOL PathWrapper::UnExpandEnvStrings(LPTSTR pszBuf, UINT cchBuf)
{
	return ::PathUnExpandEnvStrings(m_szPath, pszBuf, cchBuf);
}

BOOL PathWrapper::UnmakeSystemFolder()
{
	return ::PathUnmakeSystemFolder(m_szPath);
}

void   PathWrapper::UnquoteSpaces()
{
	::PathUnquoteSpaces(m_szPath);
}

UINT PathWrapper::GetCharType(TCHAR ch)
{
	return ::PathGetCharType(ch);
}

FileInfo::FileInfo(LPCTSTR path)
	: TFileSysInfo()
	, m_path(_T(""))
{
	FileOper::GetFileSysInfo(path, *this);
// 	// 构造路径
// 	PathWrapper pathf(path);
// 	if (pathf.IsRelative())
// 	{
// 		TCHAR temp[MAX_PATH] = { 0 };
// 		DirectoryOper::GetCurrent(temp, MAX_PATH);
// 		pathf = temp;
// 		pathf.Append(path);
// 	}
// 	pathf.Append(_T("."));  // 规整尾部'/'的情况
// 
// 	if (pathf.IsExists())
// 	{
// 		WIN32_FIND_DATA FindData;
// 		HANDLE hFind;
// 
// 		hFind = FindFirstFile(pathf, &FindData);
// 		if (hFind == INVALID_HANDLE_VALUE)
// 		{
// 			_tprintf(_T("error: %s not found!\n"), (LPCTSTR)pathf);
// 			return;
// 		}
// 		FindClose(hFind);
// 
// 		FileTimeToTime_t(FindData.ftCreationTime, &createTime);
// 		FileTimeToTime_t(FindData.ftLastAccessTime, &lastAccessTime);
// 		FileTimeToTime_t(FindData.ftLastWriteTime, &lastWriteTime);
// 		fileAttributes = FindData.dwFileAttributes;
// 		filesize = (UINT64(FindData.nFileSizeHigh) << 32) + FindData.nFileSizeLow;
// 		lstrcpy(filepath, pathf);
// 	}
}

BOOL FileInfo::IsExists() const
{
	return ::PathFileExists(filepath);
}

BOOL FileInfo::IsReadOnly() const
{
	return ((fileAttributes & FILE_ATTRIBUTE_READONLY) != 0);
}

LPCTSTR FileInfo::DirectoryName() const
{
	m_path = filepath;
	m_path.RemoveFileSpec();
	return m_path;
}

LPCTSTR FileInfo::Extension() const
{
	m_path = filepath;
	return m_path.FindExtension();
}

LPCTSTR FileInfo::FullName() const
{
	return filepath;
}

LPCTSTR FileInfo::Name() const
{
	m_path = filepath;
	return m_path.FindFileName();
}

UINT64 FileInfo::Length() const
{
	return filesize;
}

UINT64 FileInfo::Space() const
{
	return filespace;
}

time_t FileInfo::CreationTime() const
{
	return createTime;
}

time_t FileInfo::LastAccessTime() const
{
	return lastAccessTime;
}

time_t FileInfo::LastWriteTime() const
{
	return lastWriteTime;
}

DWORD FileInfo::FileAttributes() const
{
	return fileAttributes;
}

BOOL FileInfo::FileVersion(WORD *pBuffer)
{
	return FileOper::GetFileVersion(filepath, pBuffer);
}

BOOL FileInfo::Delete() const
{
	return ::DeleteFile(filepath);
}

BOOL FileInfo::Copy(LPCTSTR newFileName, BOOL overwrite) const
{
	return ::CopyFile(filepath, newFileName, overwrite==FALSE);
}

BOOL FileInfo::Rename(LPCTSTR newName) const
{
	return DirectoryOper::Rename(filepath, newName);
}

BOOL FileInfo::Move(LPCTSTR newFileName) const
{
	return ::MoveFile(filepath, newFileName);
}

//#### FileTimeToTime_t ####

time_t FileOper::FileTimeToTime_t(FILETIME ft)
{
	ULARGE_INTEGER            ui;
	ui.LowPart = ft.dwLowDateTime;
	ui.HighPart = ft.dwHighDateTime;

	//ll            =  (LONGLONG(ft.dwHighDateTime)<<32)  +  ft.dwLowDateTime;  

	return ((LONGLONG)(ui.QuadPart - 116444736000000000LL) / 10000000);
}

BOOL FileOper::GetFileVersion(LPCTSTR path, WORD *pBuffer)
{
	VS_FIXEDFILEINFO *pVi;
	DWORD dwHandle;
	int size = GetFileVersionInfoSize(path, &dwHandle);

	if (size > 0) {
		BYTE *buffer = new BYTE[size];

		if (GetFileVersionInfo(path, dwHandle, size, buffer)) {
			if (VerQueryValue(buffer, _T("\\"), (LPVOID *)&pVi, (PUINT)&size)) {
				pBuffer[0] = HIWORD(pVi->dwFileVersionMS);
				pBuffer[1] = LOWORD(pVi->dwFileVersionMS);
				pBuffer[2] = HIWORD(pVi->dwFileVersionLS);
				pBuffer[3] = LOWORD(pVi->dwFileVersionLS);

				delete buffer;
				return true;
			}
		}

		delete buffer;
	}
	return FALSE;
}

BOOL FileOper::Delete(LPCTSTR path)
{
	return ::DeleteFile(path);
}

BOOL FileOper::Copy(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite)
{
	PathWrapper path(destPath);
	if (path.IsFileSpec())
	{
		path = srcPath;
		path.RemoveBackslash();
		path.RemoveFileSpec();
		path.Append(destPath);
	}
	return ::CopyFile(srcPath, path, overwrite==FALSE);
}

BOOL FileOper::Move(LPCTSTR srcPath, LPCTSTR destPath)
{
	return ::MoveFile(srcPath, destPath);
}

BOOL FileOper::Rename(LPCTSTR srcPath, LPCTSTR newName)
{
	return DirectoryOper::Rename(srcPath, newName);
}

BOOL FileOper::IsExists(LPCTSTR path)
{
	return ::PathFileExists(path);
}

BOOL FileOper::GetFileSysInfo(LPCTSTR path, TFileSysInfo& outinfo)
{
	// 构造路径
	PathWrapper pathf(path);
	if (pathf.IsRelative())
	{
		TCHAR temp[MAX_PATH] = { 0 };
		DirectoryOper::GetCurrent(temp, MAX_PATH);
		pathf = temp;
		pathf.Append(path);
	}
	pathf.Append(_T("."));  // 规整尾部'/'的情况

	if (pathf.IsExists())
	{
		WIN32_FIND_DATA FindData;
		HANDLE hFind;

		hFind = FindFirstFile(pathf, &FindData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			_tprintf(_T("error: %s not found!\n"), (LPCTSTR)pathf);
			return FALSE;
		}
		FindClose(hFind);

		outinfo.createTime = FileTimeToTime_t(FindData.ftCreationTime);
		outinfo.lastAccessTime = FileTimeToTime_t(FindData.ftLastAccessTime);
		outinfo.lastWriteTime = FileTimeToTime_t(FindData.ftLastWriteTime);
		outinfo.fileAttributes = FindData.dwFileAttributes;
		outinfo.filesize = (UINT64(FindData.nFileSizeHigh) << 32) + FindData.nFileSizeLow;
		outinfo.filespace = (outinfo.filesize + 4095) / 4096 * 4096;
		lstrcpy(outinfo.filepath, pathf);

		return TRUE;
	}
	else 
		return FALSE;
}

TFileSysInfo FileOper::GetFileSysInfo(LPCTSTR path)
{
	TFileSysInfo info;
	GetFileSysInfo(path, info);
	return info;
}
