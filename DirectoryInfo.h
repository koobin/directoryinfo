/*!
   \ 目录及文件操作工具类集合
   \ version: v1.0.1
   \ last modified: 2020.03.11
   \ author: koobin
*/

#pragma once

#include <functional>
#include <memory>
#include <vector>

struct TFileSysInfo
{
	time_t			lastWriteTime; //ULARGE_INTEGER  FILETIME
	time_t			lastAccessTime; //ULARGE_INTEGER  FILETIME
	time_t			createTime; //ULARGE_INTEGER  FILETIME
	UINT64			filesize;   // 文件大小（文件夹为0，根目录为已用大小）
	UINT64			filespace;  // 文件占用磁盘空间（文件夹为0，根目录为可用大小）
	DWORD		fileAttributes;
	TCHAR			filepath[MAX_PATH];

	TFileSysInfo()
		: lastWriteTime(0)
		, lastAccessTime(0)
		, createTime(0)
		, filesize(0)
		, filespace(0)
		, fileAttributes(0)
	{
		ZeroMemory(filepath, sizeof(filepath));
	}
	virtual ~TFileSysInfo() { };

	virtual void Reset()
	{
		lastWriteTime = 0;
		lastAccessTime = 0;
		createTime = 0;
		filesize  = 0;
		filespace = 0;
		fileAttributes = 0;

		ZeroMemory(filepath, sizeof(filepath));
	}

// 获取指示文件是否存在的值。 
//	BOOL IsExists() const{ return ::PathFileExists(filepath); } 
// 	time_t CreationTime() const;   // 获取或设置当前 FileSystemInfo 对象的创建时间。
// 	time_t LastAccessTime() const;   // 获取或设置上次访问当前文件或目录的时间。（从 FileSystemInfo 继承。） 
// 	time_t  LastWriteTime() const;    // 获取或设置上次写入当前文件或目录的时间。（从 FileSystemInfo 继承。） 
// 	DWORD FileAttributes() const;
};

typedef std::shared_ptr<TFileSysInfo>	filesysinfo_ptr;
//typedef TFileSysInfo*							filesysinfo_ptr;
typedef std::vector<filesysinfo_ptr>		filesysinfos_t;
//typedef std::list<filesysinfo_ptr>			filesysinfos_t;

class PathWrapper;
typedef std::unique_ptr<PathWrapper>  PathWrapperPtr;

typedef std::function<void(const WIN32_FIND_DATA* , LPCTSTR )> DirectoryCallback;

// Windows SDK PathXXXX函数的包装，详细使用请参照PathXXXX函数
class PathWrapper
{
	friend class DirectoryOper;

public:
	PathWrapper();
	PathWrapper(LPCTSTR path);
	PathWrapper& operator = (LPCTSTR path);

	operator LPCTSTR() const { return m_szPath; }
	LPCTSTR Get() const { return m_szPath; }

	BOOL operator == (const PathWrapper& path);
	BOOL operator == (LPCTSTR path);

public:
	LPCTSTR AddBackslash();

	BOOL AddExtension(LPCTSTR pszExtension);

	BOOL Append( LPCTSTR pszMore);

	BOOL Canonicalize();

	void Absolute();

	int CommonPrefix(LPCTSTR pszFile, LPTSTR pszPath);

	BOOL CompactPath(LPTSTR pszOut, UINT cchMax);

	BOOL IsExists();

	LPCTSTR FindExtension();

	LPCTSTR FindFileName();

	LPCTSTR FindNextComponent();

	BOOL FindOnPath(LPTSTR pszFile);

	LPCTSTR FindSuffixArray(LPCTSTR *apszSuffix, int iArraySize);

	LPCTSTR GetArgs();

	int GetDriveNumber();

	BOOL IsContentType(LPCTSTR pszContentType);

	BOOL IsDirectory();

	BOOL IsDirectoryEmpty();

	BOOL IsFileSpec();

	BOOL IsNetworkPath();

	BOOL IsPrefix(IN LPCTSTR pszPrefix);

	BOOL IsRelative();

	BOOL IsRoot();

	BOOL IsSameRoot(LPCTSTR pszPath);

	BOOL IsSystemFolder();

	BOOL IsUNC();

	BOOL IsURL();

	BOOL MakePretty();

	BOOL MakeSystemFolder();

	BOOL MatchSpec(LPCTSTR pszSpec);

	int	 ParseIconLocation();

	void QuoteSpaces();

	void RemoveArgs();

	LPCTSTR RemoveBackslash();

	void   RemoveBlanks();

	void   RemoveExtension();

	BOOL RemoveFileSpec();

	BOOL RenameExtension(LPCTSTR pszExt);

	LPCTSTR SkipRoot();

	void   StripPath();

	BOOL StripToRoot();

	void   Undecorate();

	BOOL UnExpandEnvStrings(LPTSTR pszBuf, UINT cchBuf);

	BOOL UnmakeSystemFolder();

	void   UnquoteSpaces();

public:
	static UINT GetCharType(TCHAR ch);

private:
	TCHAR m_szPath[MAX_PATH];
};

class DirectoryInfo final: protected TFileSysInfo 
{
public:
	enum EnFilesType
	{
		FT_ALL = 0,
		FT_FILE,
		FT_DIR,
	};

public:
	DirectoryInfo() : DirectoryInfo(nullptr) { }
	//DirectoryInfo(LPCTSTR path, LPCTSTR searchPatten);
	DirectoryInfo(LPCTSTR path, LPCTSTR searchPatten = _T("*"), BOOL recursion=TRUE, DirectoryCallback func_cb=nullptr);

	virtual ~DirectoryInfo(void);

	void Refresh(LPCTSTR searchPatten = _T("*"), BOOL recursion = TRUE, DirectoryCallback func_cb = nullptr);
	BOOL Delete(BOOL recursive);
	BOOL MoveTo(LPCTSTR destDir, BOOL overwrite);
	BOOL CreateSubdir(LPCTSTR destDirName);
	BOOL IsExists() const; 
	INT64 GetSpace() const { return m_llSpace; }
	INT64 GetSize() const { return m_llSize; }
	UINT GetFiles() const { return m_nFiles; }
	UINT GetFolders() const { return m_nDirs; }
	LPCTSTR GetPath() const { return filepath; }
	LPCTSTR GetParent() const;
	LPCTSTR GetRoot() const;

	const TFileSysInfo& GetFileSysInfo() const { return *(this); }
	filesysinfos_t GetFileSysInfos(EnFilesType type = FT_ALL) const;

protected:
	//virtual void Reset();
	//virtual bool Run(const WIN32_FIND_DATA* finddata, LPCTSTR path) { return true; }

private:
	DirectoryInfo(const DirectoryInfo&);
	DirectoryInfo& operator=(const DirectoryInfo&);

	BOOL Proceed(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

	// 深度优先递归遍历目录中所有文件和文件夹(recursion==true)
	// 只遍历当前目录中所有的文件和文件夹(recursion==false)
	BOOL Traversal(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

  // 操作查询到的文件或目录
	void OperFindData(const WIN32_FIND_DATA* finddata, LPCTSTR path);

private:
	UINT m_nFiles;
	UINT m_nDirs;
	INT64 m_llSize;  // 目录下所有文件总的大小
	INT64 m_llSpace; // 占用磁盘空间
	DirectoryCallback m_cbTraversal;
	//TFileSysInfo m_FileSysInfo;
	mutable TCHAR m_szTemp[MAX_PATH];
	filesysinfos_t files;
};

class FileInfo : protected TFileSysInfo
{
public:
	FileInfo(LPCTSTR path);
	virtual ~FileInfo() { };

	BOOL IsExists() const; 
	BOOL IsReadOnly() const;  // 获取或设置确定当前文件是否为只读的值。
	LPCTSTR DirectoryName() const; // 获取表示目录的完整路径的字符串。 
	LPCTSTR Extension() const; // 获取表示文件扩展名部分的字符串。
	LPCTSTR FullName() const; // 获取目录或文件的完整目录。
	LPCTSTR Name() const;  // 获取文件名。
	UINT64 Length() const; // 获取当前文件的大小。 
	UINT64 Space() const;  // 获取当前文件的占用磁盘空间。 
	time_t CreationTime() const;   // 获取或设置当前 FileSystemInfo 对象的创建时间。
	time_t LastAccessTime() const;   // 获取或设置上次访问当前文件或目录的时间。（从 FileSystemInfo 继承。） 
	time_t  LastWriteTime() const;    // 获取或设置上次写入当前文件或目录的时间。（从 FileSystemInfo 继承。） 
	DWORD FileAttributes() const;

	BOOL FileVersion(WORD *pBuffer);
	BOOL Delete() const;
	BOOL Copy(LPCTSTR newFileName, BOOL overwrite) const;
	BOOL Rename(LPCTSTR newName) const;
	BOOL Move(LPCTSTR newFileName) const;

protected:
	
private:
	FileInfo(void);
	FileInfo(const FileInfo&);
	FileInfo& operator=(const FileInfo&);
	mutable PathWrapper m_path;
};

class DirectoryOper
{
private:
	DirectoryOper();
	DirectoryOper(const DirectoryOper&);
	DirectoryOper& operator = (const DirectoryOper&);

public:
	/////////////////////////////////////
	//输入参数：path 要创建的路径指针
	//作用：创建指定文件夹
	static BOOL Create(LPCTSTR path, BOOL recursion);  

	/////////////////////////////////////
	//输入参数：path 要删除的路径指针
	//作用：删除指定文件夹以及里面的文件
	static BOOL Delete(LPCTSTR path, BOOL recursion);  

	/////////////////////////////////////
	//输入参数：lpszFromPath 源文件夹的路径 , lpszToPath 目的文件夹的路径
	//作用：拷贝文件夹及其文件夹中的所有内容
	static BOOL Copy(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite);

	/////////////////////////////////////
	//输入参数：lpszFromPath 源文件夹路径 , lpszToPath 目的文件夹路径
	//作用：移动原文件夹及其中文件都指定的路径下
	static BOOL Move(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite);

	/////////////////////////////////////
	//输入参数：lpszFromPath 源文件夹路径 , lpszToPath 目的文件夹路径
	//作用：重命名原文件夹
	static BOOL Rename(LPCTSTR srcPath, LPCTSTR newName);

	/////////////////////////////////////
	//输入参数：path 要判断是否为文件或路径指针
	//作用：判断指定文件夹或文件是否存在 
	static BOOL IsExists(LPCTSTR path);  

	/////////////////////////////////////
	//输入参数：path 需要获取的路径指针
	//searchPatten 需要获取的文件通配符
	//recursion 是否要递归获取子目录下所有匹配文件
	//outfiles 获取的文件信息存储地（用于输出）
	//作用：获取指定目录下所有匹配文件 
	static filesysinfos_t GetFiles(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

	/////////////////////////////////////
	//输入参数：path 需要获取的路径指针
	//searchPatten 需要获取的文件夹通配符
	//recursion 是否要递归获取子目录下所有匹配文件夹
	//outdirs 获取的文件夹信息存储地（用于输出）
	//作用：获取指定目录下所有匹配文件夹 
	static filesysinfos_t GetDirectories(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

	/////////////////////////////////////
	//输入参数：path 需要获取的路径指针
	//searchPatten 需要获取的文件及文件夹 通配符
	//recursion 是否要递归获取子目录下所有匹配文件及文件夹 
	//outinfos 获取的文件系统信息存储地（用于输出）
	//作用：获取指定目录下所有匹配文件及文件夹 
	static filesysinfos_t GetFileSysInfos(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

	/////////////////////////////////////
	//输入参数：lpBuffer 获得的路径存储位置
	//作用：获取进程的当前文件夹
	static PathWrapperPtr GetCurrent();
	static BOOL GetCurrent(LPTSTR lpBuffer,  DWORD nBufferLength);

	/////////////////////////////////////
	//输入参数：lpBuffer 获得的路径存储位置
	//作用：设置进程的当前文件夹
	static BOOL SetCurrent(LPCTSTR lpPathName);

	/////////////////////////////////////
	//输入参数：lpBuffer 获得的路径存储位置
	//作用：获取进程exe文件存储的文件夹
	static PathWrapperPtr GetModule();
	static BOOL GetModule(LPTSTR lpBuffer,  DWORD nBufferLength);

};

class FileOper
{
private:
	FileOper() = delete;
	FileOper(const FileOper&) = delete;
	FileOper& operator = (const FileOper&) = delete;

public:
	static time_t FileTimeToTime_t(FILETIME ft);

	/////////////////////////////////////
	//输入参数：path 要查询的文件路径指针
	//作用：获取指定文件的版本信息，pBuffer至少包含4个元素
	static BOOL GetFileVersion(LPCTSTR path, WORD *pBuffer);

	/////////////////////////////////////
	//输入参数：path 要删除的路径指针
	//作用：删除指定文件夹以及里面的文件
	static BOOL Delete(LPCTSTR path);  

	/////////////////////////////////////
	//输入参数：lpszFromPath 源文件夹的路径 , lpszToPath 目的文件夹的路径
	//作用：拷贝文件夹及其文件夹中的所有内容
	static BOOL Copy(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite);

	/////////////////////////////////////
	//输入参数：lpszFromPath 源文件夹路径 , lpszToPath 目的文件夹路径
	//作用：移动原文件夹及其中文件都指定的路径下
	static BOOL Move(LPCTSTR srcPath, LPCTSTR destPath);

	/////////////////////////////////////
	//输入参数：lpszFromPath 源文件夹路径 , lpszToPath 目的文件夹路径
	//作用：重命名原文件夹
	static BOOL Rename(LPCTSTR srcPath, LPCTSTR newName);

	/////////////////////////////////////
	//输入参数：path 要判断是否为文件或路径指针
	//作用：判断指定文件夹或文件是否存在 
	static BOOL IsExists(LPCTSTR path);  

	/////////////////////////////////////
	//输入参数：path 需要获取的路径指针
	//outinfo 获取的文件系统信息存储地（用于输出）
	//作用：获取指定目录下所有匹配文件及文件夹 
	static BOOL GetFileSysInfo(LPCTSTR path, TFileSysInfo& outinfo);
	static TFileSysInfo GetFileSysInfo(LPCTSTR path);
};