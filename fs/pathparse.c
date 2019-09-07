#include <global.h>
/**
 * 路径解析，提取出/之间的名字
 *
 * @param path 路径名
 * @param res  遍历结果
 *
 * @return 遍历完当前项后path的位置
 * 	@retval NULL 没有东西可以提取了
 */
char *path_parse(const char *path, char *res){

	while(*path == '/')
		++path;
	
	while(*path != '/' && *path != '\0')
		*res++ = *path++;

	*res = '\0';
	if(!*path)
		return NULL;
	
	return (char *)path;
}

/**
 * 将路径分割成文件名和目录名
 *
 * @param path 路径名
 * @param filename 文件名
 * @param dirname 目录名
 *
 * @note 对于/a/b/c 分割结果为 filename : c  dirname /a/b/
 */
void split_path(const char *path, char *filename, char *dirname){
	char *pos = (char *)path;
	char *head = (char *)path;
	while(*path){
		if(*path == '/')
			pos = (char *)path + 1;
		++path;
	}

	while(head != pos) 
		*dirname++ = *head++;
	*dirname = '\0';

	while(pos != path)
		*filename++ = *pos++;
	*filename = '\0';
}

