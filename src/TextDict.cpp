/*
 * Open Chinese Convert
 *
 * Copyright 2010-2013 BYVoid <byvoid@byvoid.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "UTF8Util.hpp"
#include "TextDict.hpp"

using namespace Opencc;

#define ENTRY_BUFF_SIZE 128

shared_ptr<DictEntry> ParseKeyValues(const char* buff) {
  size_t length;
  const char* pbuff = UTF8Util::FindNextInline(buff, '\t');
  if (UTF8Util::IsLineEndingOrFileEnding(*pbuff)) {
    throw runtime_error("invalid format");
  }
  length = pbuff - buff;
  shared_ptr<DictEntry> entry(new DictEntry(UTF8Util::FromSubstr(buff, length)));
  while (!UTF8Util::IsLineEndingOrFileEnding(*pbuff)) {
    buff = pbuff = UTF8Util::NextChar(pbuff);
    pbuff = UTF8Util::FindNextInline(buff, ' ');
    length = pbuff - buff;
    string value = UTF8Util::FromSubstr(buff, length);
    entry->values.push_back(value);
  }
  return entry;
}

TextDict::TextDict() : lexicon(new vector<shared_ptr<DictEntry>>) {
  sorted = true;
}

TextDict::~TextDict() {
}

void TextDict::LoadFromFile(const string fileName) {
  FILE* fp = fopen(fileName.c_str(), "r");
  if (fp == NULL) {
    throw runtime_error("file not found");
  }
  LoadFromFile(fp);
  fclose(fp);
}

void TextDict::LoadFromFile(FILE* fp) {
  // TODO use dynamic getline
  static char buff[ENTRY_BUFF_SIZE];
  UTF8Util::SkipUtf8Bom(fp);
  
  while (fgets(buff, ENTRY_BUFF_SIZE, fp)) {
    shared_ptr<DictEntry> entry = ParseKeyValues(buff);
    AddKeyValue(entry);
  }
  SortLexicon();
}

void TextDict::LoadFromDict(Dict& dictionary) {
  lexicon = dictionary.GetLexicon();
  maxLength = dictionary.KeyMaxLength();
  sorted = true;
}

void TextDict::AddKeyValue(DictEntry entry) {
  AddKeyValue(shared_ptr<DictEntry>(new DictEntry(entry)));
}

void TextDict::AddKeyValue(shared_ptr<DictEntry> entry) {
  lexicon->push_back(entry);
  size_t keyLength = entry->key.length();
  maxLength = std::max(keyLength, maxLength);
  sorted = false;
}

void TextDict::SortLexicon() {
  if (!sorted) {
    std::sort(lexicon->begin(), lexicon->end(), DictEntry::PtrCmp);
    sorted = true;
  }
}

size_t TextDict::KeyMaxLength() const {
  return maxLength;
}

Optional<shared_ptr<DictEntry>> TextDict::MatchPrefix(const char* word) {
  SortLexicon();
  shared_ptr<DictEntry> entry(new DictEntry(UTF8Util::Truncate(word, maxLength)));
  const char* keyPtr = entry->key.c_str();
  for (long len = entry->key.length(); len > 0; len -= UTF8Util::PrevCharLength(keyPtr)) {
    entry->key.resize(len);
    keyPtr = entry->key.c_str();
    auto found = std::lower_bound(lexicon->begin(), lexicon->end(), entry, DictEntry::PtrCmp);
    if (found != lexicon->end() && (*found)->key == entry->key) {
      return Optional<shared_ptr<DictEntry>>(*found);
    }
  }
  return Optional<shared_ptr<DictEntry>>();
}

shared_ptr<vector<shared_ptr<DictEntry>>> TextDict::MatchAllPrefixes(const char* word) {
  SortLexicon();
  shared_ptr<vector<shared_ptr<DictEntry>>> matchedLengths(new vector<shared_ptr<DictEntry>>);
  shared_ptr<DictEntry> entry(new DictEntry(UTF8Util::Truncate(word, maxLength)));
  const char* keyPtr = entry->key.c_str();
  for (long len = entry->key.length(); len > 0; len -= UTF8Util::PrevCharLength(keyPtr)) {
    entry->key.resize(len);
    keyPtr = entry->key.c_str();
    auto found = std::lower_bound(lexicon->begin(), lexicon->end(), entry, DictEntry::PtrCmp);
    if (found != lexicon->end() && (*found)->key == entry->key) {
      matchedLengths->push_back(*found);
    }
  }
  return matchedLengths;
}

shared_ptr<vector<shared_ptr<DictEntry>>> TextDict::GetLexicon() {
  SortLexicon();
  return lexicon;
}

void TextDict::SerializeToFile(const string fileName) {
  FILE *fp = fopen(fileName.c_str(), "wb");
  if (fp == NULL) {
    fprintf(stderr, _("Can not write file: %s\n"), fileName.c_str());
    exit(1);
  }
  SerializeToFile(fp);
  fclose(fp);
}

void TextDict::SerializeToFile(FILE* fp) {
  SortLexicon();
  // TODO escape space
  for (auto entry : *lexicon) {
    fprintf(fp, "%s\t", entry->key.c_str());
    size_t i = 0;
    for (auto& value : entry->values) {
      fprintf(fp, "%s", value.c_str());
      if (i < entry->values.size() - 1) {
        fprintf(fp, " ");
      }
      i++;
    }
    fprintf(fp, "\n");
  }
}
