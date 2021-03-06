
#include <stdio.h>
#include <wchar.h>

#include <windows.h>
#include <objidl.h>
#include <indexsrv.h>
#include <cierror.h>
#include <filterr.h>

#include "lrsample.hxx"

// The CLSID for the wordbreaker

CLSID CLSID_SampleWordBreaker = /* d225281a-7ca9-4a46-ae7d-c63a9d4815d4 */
{
    0xd225281a,  0x7ca9, 0x4a46,
    {0xae, 0x7d, 0xc6, 0x3a, 0x9d, 0x48, 0x15, 0xd4}
};

// The CLSID of the stemmer

CLSID CLSID_SampleStemmer = /* 0a275611-aa4d-4b39-8290-4baf77703f55 */
{
	0x0a275611, 0xaa4d, 0x4b39,
	{ 0x82, 0x90, 0x4b, 0xaf, 0x77, 0x70, 0x3f, 0x55 }
};

// Global module refcount

long g_cInstances = 0;
HMODULE g_hModule = 0;

//+---------------------------------------------------------------------------
//
//  Member:     CSampleWordBreaker::BreakText
//
//  Synopsis:   Break a block of text into individual ngrams
//
//  Arguments:  [pTextSource]  -- Source of characters to work on
//              [pWordSink]    -- Where to send the words found
//              [pPhraseSink]  -- Where to send the phrases found (not used)
//
//  Returns:    S_OK if successful or an error code
//
//----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CSampleWordBreaker::BreakText(
    TEXT_SOURCE * pTextSource,
    IWordSink *   pWordSink,
    IPhraseSink * pPhraseSink )
{
    // Validate arguments

    if ( 0 == pTextSource )
        return E_INVALIDARG;

    if ( ( 0 == pWordSink ) || ( pTextSource->iCur == pTextSource->iEnd ) )
        return S_OK;

    if ( pTextSource->iCur > pTextSource->iEnd )
        return E_INVALIDARG;

    HRESULT hr = pTextSource->pfnFillTextBuffer( pTextSource );
    do
    {
        int NGRAM_SIZE = 3;
        long at = 0;
		while (pTextSource->iCur + at + NGRAM_SIZE <= pTextSource->iEnd) {
			long pos = pTextSource->iCur + at;
			//need to have a 'word', which is really just a character
			hr = pWordSink->PutWord(NGRAM_SIZE,
				&pTextSource->awcBuffer[pos],
				NGRAM_SIZE,
				pos);
			if (FAILED(hr)) return hr;
            at++;
        }


        hr = pTextSource->pfnFillTextBuffer( pTextSource );
    } while ( SUCCEEDED( hr ) );


    //
    // If anything failed except for running out of text, report the error.
    // Otherwise, for cases like out of memory, files will not get retried or
    // reported as failures properly.
    //

    if ( ( FAILED( hr ) ) &&
         ( FILTER_E_NO_MORE_VALUES != hr ) &&
         ( FILTER_E_NO_TEXT != hr ) &&
         ( FILTER_E_NO_VALUES != hr ) &&
         ( FILTER_E_NO_MORE_TEXT != hr ) &&
         ( FILTER_E_END_OF_CHUNKS != hr ) &&
         ( FILTER_E_EMBEDDING_UNAVAILABLE != hr ) &&
         ( WBREAK_E_END_OF_TEXT != hr ) )
        return hr;

	return S_OK;
} //BreakText

  //+---------------------------------------------------------------------------
  //
  //  Member:     CSampleStemmer::GenerateWordForms
  //
  //  Synopsis:   From the input word, emit the original and alternate forms
  //              of the word.
  //
  //  Arguments:  [pwcInBuf]   -- The original word to stem (not 0-terminated)
  //              [cwc]        -- Length in characters of the word
  //              [pStemSink]  -- Where to emit the stems
  //
  //  Returns:    S_OK if successful or an error code
  //
  //----------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CSampleStemmer::GenerateWordForms(
	WCHAR const *   pwcInBuf,
	ULONG           cwc,
	IWordFormSink * pWordFormSink)
{
	// Validate the arguments

	if ((0 == pwcInBuf) || (0 == pWordFormSink))
		return E_INVALIDARG;

	HRESULT hr = S_OK;
	//bigrams as stems
	ULONG at = 0;
	while (at < (cwc-1)) {
		hr = pWordFormSink->PutWord(pwcInBuf+at, 2);
		if (FAILED(hr)) return hr;
		at++;
	}

	// Emit the original word
	return pWordFormSink->PutWord(pwcInBuf, cwc);
} //StemWord


//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::CLanguageResourceSampleCF
//
//  Synopsis:   Language resource class factory constructor
//
//--------------------------------------------------------------------------

CLanguageResourceSampleCF::CLanguageResourceSampleCF() :
    _lRefs( 1 )
{
    InterlockedIncrement( &g_cInstances );
} //CLanguageResourceSampleCF

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::~CLanguageResourceSampleCF
//
//  Synopsis:   Language resource class factory destructor
//
//--------------------------------------------------------------------------

CLanguageResourceSampleCF::~CLanguageResourceSampleCF()
{
    InterlockedDecrement( &g_cInstances );
} //~LanguageResourceSampleCF

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::QueryInterface
//
//  Synopsis:   Rebind to the requested interface
//
//  Arguments:  [riid]      -- IID of new interface
//              [ppvObject] -- New interface * returned here
//
//  Returns:    S_OK if bind succeeded, E_NOINTERFACE if bind failed
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CLanguageResourceSampleCF::QueryInterface(
    REFIID   riid,
    void  ** ppvObject )
{
    if ( 0 == ppvObject )
        return E_INVALIDARG;

    if ( IID_IClassFactory == riid )
        *ppvObject = (IUnknown *) (IClassFactory *) this;
    else if ( IID_IUnknown == riid )
        *ppvObject = (IUnknown *) (IPersist *) this;
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
} //QueryInterface

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::AddRef
//
//  Synopsis:   Increments the refcount
//
//  Returns:    The new refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CLanguageResourceSampleCF::AddRef()
{
    return InterlockedIncrement( &_lRefs );
} //AddRef

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::Release
//
//  Synopsis:   Decrement refcount.  Delete self if necessary.
//
//  Returns:    The new refcount
//
//--------------------------------------------------------------------------

ULONG STDMETHODCALLTYPE CLanguageResourceSampleCF::Release()
{
    long lTmp = InterlockedDecrement( &_lRefs );

    if ( 0 == lTmp )
        delete this;

    return lTmp;
} //Release

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::CreateInstance
//
//  Synopsis:   Creates new Language Resource sample object
//
//  Arguments:  [pUnkOuter] -- 'Outer' IUnknown
//              [riid]      -- Interface to bind
//              [ppvObject] -- Interface returned here
//
//  Returns:    S_OK if successful or an appropriate error code
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CLanguageResourceSampleCF::CreateInstance(
    IUnknown * pUnkOuter,
    REFIID     riid,
    void * *   ppvObject )
{
    *ppvObject = 0;

	if (IID_IStemmer == riid)
		*ppvObject = new CSampleStemmer();
	else if (IID_IWordBreaker == riid)
		*ppvObject = new CSampleWordBreaker();
	else
		return E_NOINTERFACE;

    if ( 0 == *ppvObject )
        return E_OUTOFMEMORY;

    return S_OK;
} //CreateInstance

//+-------------------------------------------------------------------------
//
//  Method:     CLanguageResourceSampleCF::LockServer
//
//  Synopsis:   Force class factory to remain loaded
//
//  Arguments:  [fLock] -- TRUE if locking, FALSE if unlocking
//
//  Returns:    S_OK
//
//--------------------------------------------------------------------------

HRESULT STDMETHODCALLTYPE CLanguageResourceSampleCF::LockServer( BOOL fLock )
{
    if ( fLock )
        InterlockedIncrement( &g_cInstances );
    else
        InterlockedDecrement( &g_cInstances );

    return S_OK;
} //LockServer

//+-------------------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Ole DLL load class routine
//
//  Arguments:  [cid]    -- Class to load
//              [iid]    -- Interface to bind to on class object
//              [ppvObj] -- Interface pointer returned here
//
//  Returns:    Sample language resource class factory
//
//--------------------------------------------------------------------------

extern "C" HRESULT STDMETHODCALLTYPE DllGetClassObject(
    REFCLSID cid,
    REFIID   iid,
    void **  ppvObj )
{
    IUnknown * pUnk = 0;
    *ppvObj = 0;

	if (CLSID_SampleWordBreaker == cid ||
		CLSID_SampleStemmer == cid)
	{
		pUnk = new CLanguageResourceSampleCF();

		if (0 == pUnk)
			return E_OUTOFMEMORY;
	}
    else
    {
        *ppvObj = 0;
        return E_NOINTERFACE;
    }

    HRESULT hr = pUnk->QueryInterface( iid, ppvObj );

    pUnk->Release();

    return hr;
} //DllGetClassObject

//+-------------------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Notifies DLL to unload (cleanup global resources)
//
//  Returns:    S_OK if it is acceptable for caller to unload DLL.
//              S_FALSE otherwise.
//
//--------------------------------------------------------------------------

extern "C" HRESULT STDMETHODCALLTYPE DllCanUnloadNow( void )
{
    if ( 0 == g_cInstances )
        return S_OK;

    return S_FALSE;
} //DllCanUnloadNow

//+-------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Standard main entrypoint for the module.
//
//--------------------------------------------------------------------------

BOOL WINAPI DllMain(
    HANDLE hInstance,
    DWORD  dwReason,
    void * lpReserved )
{
    if ( DLL_PROCESS_ATTACH == dwReason )
    {
        g_hModule = (HMODULE) hInstance;
        DisableThreadLibraryCalls( (HINSTANCE) hInstance );
    }

    return TRUE;
} //DllMain

//+-------------------------------------------------------------------------
//
//  Method:     DllRegisterServer
//
//  Synopsis:   Registers the language resources in the registry
//
//--------------------------------------------------------------------------

STDAPI DllRegisterServer()
{
    return S_OK;
} //DllRegisterServer

//+-------------------------------------------------------------------------
//
//  Method:     DllUnregisterServer
//
//  Synopsis:   Removes the language resources from the registry
//
//--------------------------------------------------------------------------

STDAPI DllUnregisterServer()
{
    return S_OK;
} //DllUnregisterServer

