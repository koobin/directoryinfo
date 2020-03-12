/*!
   \ Ŀ¼���ļ����������༯��
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
	UINT64			filesize;   // �ļ���С���ļ���Ϊ0����Ŀ¼Ϊ���ô�С��
	UINT64			filespace;  // �ļ�ռ�ô��̿ռ䣨�ļ���Ϊ0����Ŀ¼Ϊ���ô�С��
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

// ��ȡָʾ�ļ��Ƿ���ڵ�ֵ�� 
//	BOOL IsExists() const{ return ::PathFileExists(filepath); } 
// 	time_t CreationTime() const;   // ��ȡ�����õ�ǰ FileSystemInfo ����Ĵ���ʱ�䡣
// 	time_t LastAccessTime() const;   // ��ȡ�������ϴη��ʵ�ǰ�ļ���Ŀ¼��ʱ�䡣���� FileSystemInfo �̳С��� 
// 	time_t  LastWriteTime() const;    // ��ȡ�������ϴ�д�뵱ǰ�ļ���Ŀ¼��ʱ�䡣���� FileSystemInfo �̳С��� 
// 	DWORD FileAttributes() const;
};

typedef std::shared_ptr<TFileSysInfo>	filesysinfo_ptr;
//typedef TFileSysInfo*							filesysinfo_ptr;
typedef std::vector<filesysinfo_ptr>		filesysinfos_t;
//typedef std::list<filesysinfo_ptr>			filesysinfos_t;

class PathWrapper;
typedef std::unique_ptr<PathWrapper>  PathWrapperPtr;

typedef std::function<void(const WIN32_FIND_DATA* , LPCTSTR )> DirectoryCallback;

// Windows SDK PathXXXX�����İ�װ����ϸʹ�������PathXXXX����
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

	// ������ȵݹ����Ŀ¼�������ļ����ļ���(recursion==true)
	// ֻ������ǰĿ¼�����е��ļ����ļ���(recursion==false)
	BOOL Traversal(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

  // ������ѯ�����ļ���Ŀ¼
	void OperFindData(const WIN32_FIND_DATA* finddata, LPCTSTR path);

private:
	UINT m_nFiles;
	UINT m_nDirs;
	INT64 m_llSize;  // Ŀ¼�������ļ��ܵĴ�С
	INT64 m_llSpace; // ռ�ô��̿ռ�
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
	BOOL IsReadOnly() const;  // ��ȡ������ȷ����ǰ�ļ��Ƿ�Ϊֻ����ֵ��
	LPCTSTR DirectoryName() const; // ��ȡ��ʾĿ¼������·�����ַ����� 
	LPCTSTR Extension() const; // ��ȡ��ʾ�ļ���չ�����ֵ��ַ�����
	LPCTSTR FullName() const; // ��ȡĿ¼���ļ�������Ŀ¼��
	LPCTSTR Name() const;  // ��ȡ�ļ�����
	UINT64 Length() const; // ��ȡ��ǰ�ļ��Ĵ�С�� 
	UINT64 Space() const;  // ��ȡ��ǰ�ļ���ռ�ô��̿ռ䡣 
	time_t CreationTime() const;   // ��ȡ�����õ�ǰ FileSystemInfo ����Ĵ���ʱ�䡣
	time_t LastAccessTime() const;   // ��ȡ�������ϴη��ʵ�ǰ�ļ���Ŀ¼��ʱ�䡣���� FileSystemInfo �̳С��� 
	time_t  LastWriteTime() const;    // ��ȡ�������ϴ�д�뵱ǰ�ļ���Ŀ¼��ʱ�䡣���� FileSystemInfo �̳С��� 
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
	//���������path Ҫ������·��ָ��
	//���ã�����ָ���ļ���
	static BOOL Create(LPCTSTR path, BOOL recursion);  

	/////////////////////////////////////
	//���������path Ҫɾ����·��ָ��
	//���ã�ɾ��ָ���ļ����Լ�������ļ�
	static BOOL Delete(LPCTSTR path, BOOL recursion);  

	/////////////////////////////////////
	//���������lpszFromPath Դ�ļ��е�·�� , lpszToPath Ŀ���ļ��е�·��
	//���ã������ļ��м����ļ����е���������
	static BOOL Copy(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite);

	/////////////////////////////////////
	//���������lpszFromPath Դ�ļ���·�� , lpszToPath Ŀ���ļ���·��
	//���ã��ƶ�ԭ�ļ��м������ļ���ָ����·����
	static BOOL Move(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite);

	/////////////////////////////////////
	//���������lpszFromPath Դ�ļ���·�� , lpszToPath Ŀ���ļ���·��
	//���ã�������ԭ�ļ���
	static BOOL Rename(LPCTSTR srcPath, LPCTSTR newName);

	/////////////////////////////////////
	//���������path Ҫ�ж��Ƿ�Ϊ�ļ���·��ָ��
	//���ã��ж�ָ���ļ��л��ļ��Ƿ���� 
	static BOOL IsExists(LPCTSTR path);  

	/////////////////////////////////////
	//���������path ��Ҫ��ȡ��·��ָ��
	//searchPatten ��Ҫ��ȡ���ļ�ͨ���
	//recursion �Ƿ�Ҫ�ݹ��ȡ��Ŀ¼������ƥ���ļ�
	//outfiles ��ȡ���ļ���Ϣ�洢�أ����������
	//���ã���ȡָ��Ŀ¼������ƥ���ļ� 
	static filesysinfos_t GetFiles(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

	/////////////////////////////////////
	//���������path ��Ҫ��ȡ��·��ָ��
	//searchPatten ��Ҫ��ȡ���ļ���ͨ���
	//recursion �Ƿ�Ҫ�ݹ��ȡ��Ŀ¼������ƥ���ļ���
	//outdirs ��ȡ���ļ�����Ϣ�洢�أ����������
	//���ã���ȡָ��Ŀ¼������ƥ���ļ��� 
	static filesysinfos_t GetDirectories(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

	/////////////////////////////////////
	//���������path ��Ҫ��ȡ��·��ָ��
	//searchPatten ��Ҫ��ȡ���ļ����ļ��� ͨ���
	//recursion �Ƿ�Ҫ�ݹ��ȡ��Ŀ¼������ƥ���ļ����ļ��� 
	//outinfos ��ȡ���ļ�ϵͳ��Ϣ�洢�أ����������
	//���ã���ȡָ��Ŀ¼������ƥ���ļ����ļ��� 
	static filesysinfos_t GetFileSysInfos(LPCTSTR path, LPCTSTR searchPatten, BOOL recursion);

	/////////////////////////////////////
	//���������lpBuffer ��õ�·���洢λ��
	//���ã���ȡ���̵ĵ�ǰ�ļ���
	static PathWrapperPtr GetCurrent();
	static BOOL GetCurrent(LPTSTR lpBuffer,  DWORD nBufferLength);

	/////////////////////////////////////
	//���������lpBuffer ��õ�·���洢λ��
	//���ã����ý��̵ĵ�ǰ�ļ���
	static BOOL SetCurrent(LPCTSTR lpPathName);

	/////////////////////////////////////
	//���������lpBuffer ��õ�·���洢λ��
	//���ã���ȡ����exe�ļ��洢���ļ���
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
	//���������path Ҫ��ѯ���ļ�·��ָ��
	//���ã���ȡָ���ļ��İ汾��Ϣ��pBuffer���ٰ���4��Ԫ��
	static BOOL GetFileVersion(LPCTSTR path, WORD *pBuffer);

	/////////////////////////////////////
	//���������path Ҫɾ����·��ָ��
	//���ã�ɾ��ָ���ļ����Լ�������ļ�
	static BOOL Delete(LPCTSTR path);  

	/////////////////////////////////////
	//���������lpszFromPath Դ�ļ��е�·�� , lpszToPath Ŀ���ļ��е�·��
	//���ã������ļ��м����ļ����е���������
	static BOOL Copy(LPCTSTR srcPath, LPCTSTR destPath, BOOL overwrite);

	/////////////////////////////////////
	//���������lpszFromPath Դ�ļ���·�� , lpszToPath Ŀ���ļ���·��
	//���ã��ƶ�ԭ�ļ��м������ļ���ָ����·����
	static BOOL Move(LPCTSTR srcPath, LPCTSTR destPath);

	/////////////////////////////////////
	//���������lpszFromPath Դ�ļ���·�� , lpszToPath Ŀ���ļ���·��
	//���ã�������ԭ�ļ���
	static BOOL Rename(LPCTSTR srcPath, LPCTSTR newName);

	/////////////////////////////////////
	//���������path Ҫ�ж��Ƿ�Ϊ�ļ���·��ָ��
	//���ã��ж�ָ���ļ��л��ļ��Ƿ���� 
	static BOOL IsExists(LPCTSTR path);  

	/////////////////////////////////////
	//���������path ��Ҫ��ȡ��·��ָ��
	//outinfo ��ȡ���ļ�ϵͳ��Ϣ�洢�أ����������
	//���ã���ȡָ��Ŀ¼������ƥ���ļ����ļ��� 
	static BOOL GetFileSysInfo(LPCTSTR path, TFileSysInfo& outinfo);
	static TFileSysInfo GetFileSysInfo(LPCTSTR path);
};