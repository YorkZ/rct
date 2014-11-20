#include <rct/Serializer.h>

namespace DBMapHelpers {
template <typename Value>
static inline void deserializeValue(Deserializer &deserializer, std::shared_ptr<Value> &value)
{
    value = std::make_shared<Value>();
    deserializer >> *value.get();
}

template <typename Value>
static inline void deserializeValue(Deserializer &deserializer, Value &value)
{
    deserializer >> value;
}
template <typename T>
static inline void serializeValue(Serializer &serializer, const std::shared_ptr<T> &value)
{
    assert(value.get());
    serializer << *value.get();
}

template <typename T>
static inline void serializeValue(Serializer &serializer, const T &value)
{
    serializer << value;
}
}

template <typename Key, typename Value>
DB<Key, Value>::DB()
    : mVersion(0), mWriteScope(0)
{
}

template <typename Key, typename Value>
DB<Key, Value>::DB(DB<Key, Value> &&other)
    : mVersion(other.mVersion), mWriteScope(other.mWriteScope), mMap(std::move(other.mMap))
{
    other.mVersion = 0;
    other.mWriteScope = 0;
}

template <typename Key, typename Value>
DB<Key, Value>::~DB()
{
    assert(!mWriteScope);
}

template <typename Key, typename Value>
DB<Key, Value> &DB<Key, Value>::operator=(DB<Key, Value> &&other)
{
    mWriteScope = other.mWriteScope;
    other.mWriteScope = 0;
    mMap = std::move(other.mMap);
    return *this;
}

template <typename Key, typename Value>
bool DB<Key, Value>::open(const Path &path, uint16_t version, unsigned int flags)
{
    FILE *f = 0;
    if (!(flags & Overwrite)) {
        f = fopen(path.constData(), "r");
        if (f) {
            bool ok = false;
            fseek(f, 0, SEEK_END);
            const int size = ftell(f);
            if (size) {
                fseek(f, 0, SEEK_SET);
                char *buf = new char[size];
                const int ret = fread(buf, sizeof(char), size, f);
                if (ret == size) {
                    Deserializer deserializer(buf, size);
                    uint16_t ver;
                    deserializer >> ver;
                    if (ver == version) {
                        int size;
                        deserializer >> size;
                        // error() << "reading" << size << "for" << path;
                        Key key;
                        while (size--) {
                            deserializer >> key;
                            DBMapHelpers::deserializeValue(deserializer, mMap[key]);
                        }
                        ok = true;
                    } else {
                        error() << "Failed to load" << path << "Wrong version, expected" << version << "got" << ver;
                    }
                } else {
                    error() << "Failed to read" << path;
                }
                delete[] buf;
                // error() << "Read" << this->size() << "items";
            }
            if (!ok) {
                fclose(f);
                f = 0;
            }
        }
    }
    if (!f) {
        Path::mkdir(path.parentDir(), Path::Recursive);
        f = fopen(path.constData(), "w");
    }
    if (f) {
        fclose(f);
        mPath = path;
        mVersion = version;
        return true;
    }
    return false;
}

template <typename Key, typename Value>
void DB<Key, Value>::close()
{
    mPath.clear();
    mMap.clear();
}

template <typename Key, typename Value>
Value DB<Key, Value>::value(const Key &key) const
{
    return mMap.value(key);
}

template <typename Key, typename Value>
const Key &DB<Key, Value>::iterator::key() const
{
    return mIterator.first;
}

template <typename Key, typename Value>
const Value &DB<Key, Value>::iterator::constValue() const
{
    return mIterator.second;
}

template <typename Key, typename Value>
const Value &DB<Key, Value>::iterator::value() const
{
    return mIterator.second;
}

template <typename Key, typename Value>
void DB<Key, Value>::iterator::setValue(const Value &value)
{
    assert(mDB->mWriteScope);
    mIterator->second = value;
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator &DB<Key, Value>::iterator::operator++()
{
    ++mIterator;
    return *this;
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator &DB<Key, Value>::iterator::operator--()
{
    --mIterator;
    return *this;
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator DB<Key, Value>::iterator::operator++(int)
{
    assert(mIterator != mDB->end().mIterator);
    const auto prev = mIterator;
    ++mIterator;
    return iterator(mDB, prev);
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator DB<Key, Value>::iterator::operator--(int)
{
    assert(mIterator != mDB->begin().mIterator);
    const auto prev = mIterator;
    --mIterator;
    return iterator(mDB, prev);
}

template <typename Key, typename Value>
const typename std::pair<const Key, Value> *DB<Key, Value>::iterator::operator->() const
{
    return mIterator.operator->();
}

template <typename Key, typename Value>
const typename std::pair<const Key, Value> &DB<Key, Value>::iterator::operator*() const
{
    return mIterator.operator*();
}

template <typename Key, typename Value>
bool DB<Key, Value>::iterator::operator==(const iterator &other) const
{
    return mIterator == other.mIterator;
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator DB<Key, Value>::begin()
{
    return iterator(this, mMap.begin());
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator DB<Key, Value>::end()
{
    return iterator(this, mMap.end());
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator DB<Key, Value>::lower_bound(const Key &key)
{
    return iterator(this, mMap.lower_bound(key));
}

template <typename Key, typename Value>
typename DB<Key, Value>::iterator DB<Key, Value>::find(const Key &key)
{
    return iterator(this, mMap.find(key));
}

template <typename Key, typename Value>
const Value &DB<Key, Value>::operator[](const Key &key) const
{
    return mMap[key];
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator DB<Key, Value>::begin() const
{
    return mMap.begin();
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator DB<Key, Value>::end() const
{
    return mMap.end();
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator DB<Key, Value>::lower_bound(const Key &key) const
{
    return mMap.lower_bound(key);
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator DB<Key, Value>::find(const Key &key) const
{
    return mMap.find(key);
}

template <typename Key, typename Value>
const Key &DB<Key, Value>::const_iterator::key() const
{
    return mIterator.first;
}

template <typename Key, typename Value>
const Value &DB<Key, Value>::const_iterator::constValue() const
{
    return mIterator.second;
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator &DB<Key, Value>::const_iterator::operator++()
{
    ++mIterator;
    return *this;
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator &DB<Key, Value>::const_iterator::operator--()
{
    --mIterator;
    return *this;
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator DB<Key, Value>::const_iterator::operator++(int)
{
    const auto const_iterator = mIterator;
    ++mIterator;
    return const_iterator;
}

template <typename Key, typename Value>
typename DB<Key, Value>::const_iterator DB<Key, Value>::const_iterator::operator--(int)
{
    const auto const_iterator = mIterator;
    --mIterator;
    return const_iterator;
}

template <typename Key, typename Value>
const typename std::pair<const Key, Value> *DB<Key, Value>::const_iterator::operator->() const
{
    return mIterator.operator->();
}

template <typename Key, typename Value>
const typename std::pair<const Key, Value> &DB<Key, Value>::const_iterator::operator*() const
{
    return mIterator.operator*();
}

template <typename Key, typename Value>
bool DB<Key, Value>::const_iterator::operator==(const const_iterator &other) const
{
    return mIterator == other.mIterator;
}

template <typename Key, typename Value>
DB<Key, Value>::WriteScope::WriteScope(DB *db, int /*reservedSize*/)
    : mDB(db)
{
    ++mDB->mWriteScope;
}

template <typename Key, typename Value>
DB<Key, Value>::WriteScope::~WriteScope()
{
    if (mDB && !--mDB->mWriteScope) {
        mDB->write();
    }
}

template <typename Key, typename Value>
bool DB<Key, Value>::WriteScope::flush(String *error)
{
    assert(mDB);
    assert(mDB->mWriteScope == 1);
    --mDB->mWriteScope;
    const bool ret = mDB->write(error);
    mDB = 0;
    return ret;
}

template <typename Key, typename Value>
int DB<Key, Value>::size() const
{
    return mMap.size();
}

template <typename Key, typename Value>
bool DB<Key, Value>::write(String *error)
{
    assert(!mWriteScope);
    Path::mkdir(mPath.parentDir(), Path::Recursive);
    FILE *f = fopen(mPath.constData(), "w");
    if (!f) {
        if (error)
            *error = String::format<64>("Failed to open %s for writing: %d", mPath.constData(), errno);
        return false;
    }

    Serializer serializer(f);
    serializer << mVersion;
    serializer << size();
    for (const auto &it : mMap) {
        serializer << it.first;
        DBMapHelpers::serializeValue(serializer, it.second);
    }

    fclose(f);
    return true;
}

template <typename Key, typename Value>
void DB<Key, Value>::set(const Key &key, const Value &value)
{
    assert(mWriteScope);
    mMap[key] = value;
}

template <typename Key, typename Value>
bool DB<Key, Value>::remove(const Key &key)
{
    assert(mWriteScope);
    return mMap.remove(key);
}

template <typename Key, typename Value>
void DB<Key, Value>::erase(const typename DB<Key, Value>::iterator &it)
{
    assert(mWriteScope);
    mMap.erase(it.mIterator);
}
