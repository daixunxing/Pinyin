<?php
require 'php/pinyin.class.php';

/* 设置拼音对照表，在这里我们使用已定义的默认路径 PY_TABLE_DEFAULT
if (pinyin::settable(dirname(__FILE__).'/../table/gb2312.gb') === false)
	exit("error: 未找到拼音对照表文件\n");
*/

// 字符串 转换至 拼音
$str = '你好,世界! GB2312中没有“翯”这个字';
echo $str."\n";
$str = iconv('UTF-8', 'GBK//IGNORE', $str);
$res = pinyin::convstr($str, pinyin::PY_SPACE);
$res = iconv('GBK', 'UTF-8//IGNORE', $res);
echo $res."\n";

// 拼音 转换至 字符串
$pinyin = 'wang';
$charlist = pinyin::getgbchar($pinyin);
foreach ($charlist as &$char) {
	$char = iconv('GBK', 'UTF-8', $char);
}
print_r($charlist);
