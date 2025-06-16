#pragma once
#include <string>
#include <vector>

using Headers = std::vector<std::string>;

std::string httpPost(const std::string& url, const std::string& data, const Headers& headers = {});
std::string httpGet(const std::string& url, const Headers& headers = {});
