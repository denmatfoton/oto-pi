#include "Zone.h"

#include <iostream>
#include <fstream>

using namespace std;

template<typename T>
static void WriteBasic(ofstream& out, T value)
{
    out.write(reinterpret_cast<const char*>(&value), sizeof(value));
}

template<typename T>
static void ReadBasic(ifstream& in, T& value)
{
    in.read(reinterpret_cast<char*>(&value), sizeof(value));
}

int Irrigation::Zone::SaveToFile(const char* fileName) const
{
    try
    {
        ofstream out;
        out.open(fileName, ios::out | ios::binary);

        WriteBasic(out, m_type);
        WriteBasic(out, static_cast<int>(m_points.size()));

        for (auto& point : m_points)
        {
            WriteBasic(out, point.r);
            WriteBasic(out, point.fi);
        }
    }
    catch (const ofstream::failure& e)
    {
        cout << "Zone::SaveToFile exception: " << e.what() << endl;
        return -1;
    }

    return 0;
}

int Irrigation::Zone::LoadFromFile(const char* fileName)
{
    try
    {
        ifstream in;
        in.open(fileName, ios::in | ios::binary);

        ReadBasic(in, m_type);

        int n = 0;
        ReadBasic(in, n);
        m_points.resize(n);

        for (auto& point : m_points)
        {
            ReadBasic(in, point.r);
            ReadBasic(in, point.fi);
        }
    }
    catch (const ifstream::failure& e)
    {
        cout << "Zone::LoadFromFile exception: " << e.what() << endl;
        return -1;
    }

    return 0;
}
