/*******************************************************************************
* This program and the accompanying materials
* are made available under the terms of the Common Public License v1.0
* which accompanies this distribution, and is available at 
* http://www.eclipse.org/legal/cpl-v10.html
* 
* Contributors:
*     Peter Smith
*******************************************************************************/

#include "common/Registry.h"
#include "common/Log.h"
#include "java\JNI.h"

jlong Registry::OpenKey(JNIEnv* env, jobject /*self*/, jlong rootKey, jstring subKey, bool readOnly)
{
	jboolean iscopy = false;
	const char* sk = subKey ? env->GetStringUTFChars(subKey, &iscopy) : 0;
	HKEY key;
	DWORD access = readOnly ? KEY_READ : KEY_ALL_ACCESS;
#ifdef X64
	access |= KEY_WOW64_64KEY;
#else
	access |= KEY_WOW64_32KEY;
#endif
	LONG result = RegOpenKeyEx((HKEY) rootKey, sk, 0, access, &key);

	if(subKey) env->ReleaseStringUTFChars(subKey, sk);

	if(result == ERROR_SUCCESS) {
		return (jlong) key;	
	} else {
		return 0;
	}
}

jlong Registry::CreateSubKey(JNIEnv* env, jobject /*self*/, jlong rootKey, jstring subKey)
{
	jboolean iscopy = false;
	const char* sk = subKey ? env->GetStringUTFChars(subKey, &iscopy) : 0;
	HKEY key;
	DWORD access = KEY_ALL_ACCESS;
#ifdef X64
	access |= KEY_WOW64_64KEY;
#endif
	LONG result = RegCreateKeyEx((HKEY) rootKey, sk, 0, 0, 0, access, 0, &key, 0);

	if(subKey) env->ReleaseStringUTFChars(subKey, sk);

	if(result == ERROR_SUCCESS) {
		return (jlong) key;	
	} else {
		return 0;
	}
}


void Registry::CloseKey(JNIEnv* /*env*/, jobject /*self*/, jlong handle)
{
	if(handle == 0)
		return;
	RegCloseKey((HKEY) handle);
}

jobjectArray Registry::GetSubKeyNames(JNIEnv* env, jobject /*self*/, jlong handle)
{
	if(handle == 0)
		return 0;

	DWORD keyCount = 0;
	LONG result = RegQueryInfoKey((HKEY) handle, NULL, NULL, NULL, &keyCount, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
	if(result != ERROR_SUCCESS) {
		return NULL;
	}

	char tmp[MAX_PATH];

	jclass clazz = env->FindClass("java/lang/String");
	jobjectArray arr = env->NewObjectArray(keyCount, clazz, NULL);

	for(DWORD i = 0; i < keyCount; i++) {
		DWORD size = MAX_PATH;
		RegEnumKeyEx((HKEY) handle, i, tmp, &size, 0, 0, 0, 0);	
		env->SetObjectArrayElement(arr, i, env->NewStringUTF(tmp));
	}

	return arr;
}

jobjectArray Registry::GetValueNames(JNIEnv* env, jobject /*self*/, jlong handle)
{
	if(handle == 0)
		return 0;

	DWORD valueCount = 0;
	LONG result = RegQueryInfoKey((HKEY) handle, NULL, NULL, NULL, NULL, NULL, NULL, &valueCount, NULL, NULL, NULL, NULL);
	if(result != ERROR_SUCCESS) {
		return NULL;
	}

	char tmp[MAX_PATH];

	jclass clazz = env->FindClass("java/lang/String");
	jobjectArray arr = env->NewObjectArray(valueCount, clazz, NULL);

	for(DWORD i = 0; i < valueCount; i++) {
		DWORD size = MAX_PATH;
		RegEnumValue((HKEY) handle, i, tmp, &size, 0, 0, 0, 0);	
		env->SetObjectArrayElement(arr, i, env->NewStringUTF(tmp));
	}

	return arr;
}

void Registry::DeleteSubKey(JNIEnv* env, jobject /*self*/, jlong handle, jstring subKey)
{
	const char* sk = subKey ? env->GetStringUTFChars(subKey, 0) : 0;
	if(handle != 0 && sk != 0)
#ifdef X64
		RegDeleteKeyEx((HKEY) handle, sk, KEY_WOW64_64KEY, 0);
#else
		RegDeleteKey((HKEY) handle, sk);
#endif
	if(sk) env->ReleaseStringUTFChars(subKey, sk);
}

void Registry::DeleteValue(JNIEnv* env, jobject /*self*/, jlong parent, jstring name)
{
	if(parent == 0)
		return;

	jboolean iscopy = false;
	const char* str = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	RegDeleteValue((HKEY) parent, str);
	env->ReleaseStringUTFChars(name, str);
}

jlong Registry::GetType(JNIEnv* env, jobject /*self*/, jlong parent, jstring name)
{
	if(parent == 0)
		return 0;

	jboolean iscopy = false;
	const char* str = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	DWORD type = 0;
	LONG result = RegQueryValueEx((HKEY) parent, str, NULL, &type, NULL, NULL);
	env->ReleaseStringUTFChars(name, str);

	if(result == ERROR_SUCCESS)
		return type;
	else 
		return 0;
}

jstring Registry::GetString(JNIEnv* env, jobject /*self*/, jlong parent, jstring name)
{
	if(parent == 0)
		return 0;

	jboolean iscopy = false;
	const char* str = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	DWORD type = 0;
	TCHAR buffer[4096];
	DWORD len = 4096;
	LONG result = RegQueryValueEx((HKEY) parent, str, 0, &type, (LPBYTE) buffer, &len);
	env->ReleaseStringUTFChars(name, str);

	if(result == ERROR_SUCCESS)
		return env->NewStringUTF((char *) buffer);
	else 
		return 0;
}

jbyteArray Registry::GetBinary(JNIEnv* env, jobject /*self*/, jlong parent, jstring name)
{
	if(parent == 0)
		return 0;

	jboolean iscopy = false;
	const char* str = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	DWORD type = REG_BINARY;
	TCHAR buffer[4096];
	DWORD len = 4096;
	LONG result = RegQueryValueEx((HKEY) parent, str, 0, &type, (LPBYTE) buffer, &len);
	env->ReleaseStringUTFChars(name, str);

	if(result == ERROR_SUCCESS) {
		jbyteArray arr = env->NewByteArray(len);
		env->SetByteArrayRegion(arr, 0, len, (jbyte *)buffer);
		return arr;
	}
	else 
		return 0;
}

jlong Registry::GetDoubleWord(JNIEnv* env, jobject /*self*/, jlong parent, jstring name)
{
	if(parent == 0)
		return 0;

	jboolean iscopy = false;
	const char* str = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	DWORD type = REG_DWORD;
	DWORD value = 0;
	DWORD len = sizeof(DWORD);
	LONG result = RegQueryValueEx((HKEY) parent, str, 0, &type, (LPBYTE) &value, &len);
	env->ReleaseStringUTFChars(name, str);

	if(result == ERROR_SUCCESS)
		return value;
	else 
		return 0;
}

jstring Registry::GetExpandedString(JNIEnv* env, jobject /*self*/, jlong parent, jstring name)
{
	if(parent == 0)
		return 0;

	jboolean iscopy = false;
	const char* str = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	DWORD type = REG_EXPAND_SZ;
	TCHAR buffer[4096];
	DWORD len = 4096;
	LONG result = RegQueryValueEx((HKEY) parent, str, 0, &type, (LPBYTE) buffer, &len);
	env->ReleaseStringUTFChars(name, str);

	if(result == ERROR_SUCCESS) {
		TCHAR nb[4096];
		ExpandEnvironmentStrings(buffer, nb, len);
		return env->NewStringUTF((char *) nb);
	}
	else 
		return 0;
}

jobjectArray Registry::GetMultiString(JNIEnv* env, jobject /*self*/, jlong parent, jstring name)
{
	if(parent == 0)
		return 0;

	jboolean iscopy = false;
	const char* str = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	DWORD type = REG_MULTI_SZ;
	TCHAR buffer[4096];
	DWORD len = 4096;
	LONG result = RegQueryValueEx((HKEY) parent, str, 0, &type, (LPBYTE) buffer, &len);
	env->ReleaseStringUTFChars(name, str);

	if(result == ERROR_SUCCESS) {
		return 0; // TODO convert result
	}
	else 
		return 0;
}

void Registry::SetString(JNIEnv* env, jobject /*self*/, jlong parent, jstring name, jstring value)
{
	if(parent == 0) return;
	jboolean iscopy = false;
	const char* nameStr = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	const char* valueStr = value ? env->GetStringUTFChars(value, &iscopy) : 0;
	RegSetValueEx((HKEY) parent, nameStr, 0, REG_SZ, (const BYTE*) valueStr, strlen(valueStr));
	env->ReleaseStringUTFChars(name, nameStr);
	env->ReleaseStringUTFChars(value, valueStr);
}

void Registry::SetBinary(JNIEnv* env, jobject /*self*/, jlong parent, jstring name, jarray value)
{
	if(parent == 0) return;
	jboolean iscopy = false;
	const char* nameStr = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	void* data = env->GetPrimitiveArrayCritical(value, &iscopy);
	RegSetValueEx((HKEY) parent, nameStr, 0, REG_BINARY, (const BYTE*) data, env->GetArrayLength(value));
	env->ReleasePrimitiveArrayCritical(value, data, 0);
	env->ReleaseStringUTFChars(name, nameStr);
}

void Registry::SetDoubleWord(JNIEnv* env, jobject /*self*/, jlong parent, jstring name, jlong value)
{
	if(parent == 0) return;
	jboolean iscopy = false;
	const char* nameStr = name ? env->GetStringUTFChars(name, &iscopy) : 0;
	DWORD v = value;
	RegSetValueEx((HKEY) parent, nameStr, 0, REG_DWORD, (const BYTE*) &v, sizeof(DWORD));
	env->ReleaseStringUTFChars(name, nameStr);
}

void Registry::SetMultiString(JNIEnv* env, jobject /*self*/, jlong parent, jstring name, jobjectArray value)
{
}

bool Registry::RegisterNatives(JNIEnv *env)
{
	Log::Info("Registering natives for Registry class");
	jclass clazz = JNI::FindClass(env, "org/boris/winrun4j/RegistryKey");
	if(clazz == NULL) {
		Log::Warning("Could not find RegistryKey class");
		if(env->ExceptionCheck())
			env->ExceptionClear();
		return false;
	}

	JNINativeMethod methods[17];
	methods[0].fnPtr = (void*) CloseKey;
	methods[0].name = "closeKeyHandle";
	methods[0].signature = "(J)V";
	methods[1].fnPtr = (void*) DeleteSubKey;
	methods[1].name = "deleteSubKey";
	methods[1].signature = "(JLjava/lang/String;)V";
	methods[2].fnPtr = (void*) DeleteValue;
	methods[2].name = "deleteValue";
	methods[2].signature = "(JLjava/lang/String;)V";
	methods[3].fnPtr = (void*) GetBinary;
	methods[3].name = "getBinary";
	methods[3].signature = "(JLjava/lang/String;)[B";
	methods[4].fnPtr = (void*) GetDoubleWord;
	methods[4].name = "getDoubleWord";
	methods[4].signature = "(JLjava/lang/String;)J";
	methods[5].fnPtr = (void*) GetExpandedString;
	methods[5].name = "getExpandedString";
	methods[5].signature = "(JLjava/lang/String;)Ljava/lang/String;";
	methods[6].fnPtr = (void*) GetMultiString;
	methods[6].name = "getMultiString";
	methods[6].signature = "(JLjava/lang/String;)[Ljava/lang/String;";
	methods[7].fnPtr = (void*) GetString;
	methods[7].name = "getString";
	methods[7].signature = "(JLjava/lang/String;)Ljava/lang/String;";
	methods[8].fnPtr = (void*) GetSubKeyNames;
	methods[8].name = "getSubKeyNames";
	methods[8].signature = "(J)[Ljava/lang/String;";
	methods[9].fnPtr = (void*) GetType;
	methods[9].name = "getType";
	methods[9].signature = "(JLjava/lang/String;)J";
	methods[10].fnPtr = (void*) GetValueNames;
	methods[10].name = "getValueNames";
	methods[10].signature = "(J)[Ljava/lang/String;";
	methods[11].fnPtr = (void*) OpenKey;
	methods[11].name = "openKeyHandle";
	methods[11].signature = "(JLjava/lang/String;Z)J";
	methods[12].fnPtr = (void*) SetBinary;
	methods[12].name = "setBinary";
	methods[12].signature = "(JLjava/lang/String;[B)V";
	methods[13].fnPtr = (void*) SetDoubleWord;
	methods[13].name = "setDoubleWord";
	methods[13].signature = "(JLjava/lang/String;J)V";
	methods[14].fnPtr = (void*) SetMultiString;
	methods[14].name = "setMultiString";
	methods[14].signature = "(JLjava/lang/String;[Ljava/lang/String;)V";
	methods[15].fnPtr = (void*) SetString;
	methods[15].name = "setString";
	methods[15].signature = "(JLjava/lang/String;Ljava/lang/String;)V";
	methods[16].fnPtr = (void*) CreateSubKey;
	methods[16].name = "createSubKey";
	methods[16].signature = "(JLjava/lang/String;)J";
	
	env->RegisterNatives(clazz, methods, 17);
	if(env->ExceptionOccurred()) {
		JNI::PrintStackTrace(env);
		return false;
	}

	return true;
}
