#include <bits/stdc++.h>
using namespace std;

// code adapted from https://cp-algorithms.com/string/suffix-automaton.html
// for a string of length n and an alphabet of size k, time for build is O(nlogk) and memory is O(n)

struct state
{
    int len, link;
    map<char, int> next;
};
class SuffixAutomaton
{
    int text_size;
    vector<state> st;
    int sz, last;
    const string TEXT;
    void sa_init()
    {
        st[0].len = 0;
        st[0].link = -1;
        sz = 1;
        last = 0;
    }
    void sa_extend(char c)
    {
        int cur = sz++;
        st[cur].len = st[last].len + 1;
        int p = last;
        while (p != -1 && !st[p].next.count(c))
        {
            st[p].next[c] = cur;
            p = st[p].link;
        }
        if (p == -1)
        {
            st[cur].link = 0;
        }
        else
        {
            int q = st[p].next[c];
            if (st[p].len + 1 == st[q].len)
            {
                st[cur].link = q;
            }
            else
            {
                int clone = sz++;
                st[clone].len = st[p].len + 1;
                st[clone].next = st[q].next;
                st[clone].link = st[q].link;
                while (p != -1 && st[p].next[c] == q)
                {
                    st[p].next[c] = clone;
                    p = st[p].link;
                }
                st[q].link = st[cur].link = clone;
            }
        }
        last = cur;
    }

public:
    SuffixAutomaton(const string &text): TEXT(text),text_size(text.size())
    {
        st.resize(2 * text_size + 10);
        sa_init();
        for (auto c : text)
            sa_extend(c);
    }

    int longest_prefix(string pattern)
    {
        int cur = 0;
        int len = 0;
        for (auto c : pattern)
        {
            if (st[cur].next.count(c))
            {
                cur = st[cur].next[c];
                len++;
            }
            else
            {
                return len;
            }
        }
        return len;
    }
    string longest_substring(string pattern)
    {
        int cur = 0;
        int len = 0;
        int temp = 0;
        string ans;

        for (int i = 0; i < pattern.size(); i++)
        {
            char c = pattern[i];
            if (st[cur].next.count(c))
            {
                temp++;
                cur = st[cur].next[c];
            }
            else
            {
                while (~cur and !st[cur].next.count(c))
                {
                    cur = st[cur].link;
                }
                if (~cur)
                {
                    temp = st[cur].len + 1;
                    cur = st[cur].next[c];
                }
                else
                {
                    cur = 0;
                    temp = 0;
                }
            }
            // i - temp + 1 is start of match
            if (len < temp)
            {
                ans = pattern.substr(i - temp + 1, temp);
                len = temp;
            }
        }
        return ans;
    }
};
int main()
{
    string s;
    cout << "Enter the string: " << endl;
    cin >> s;
    string pat;
    cin >> pat;
    SuffixAutomaton sa(s);
    cout << "The legnth of the longest prefix of pattern is: " << sa.longest_prefix(pat) << endl;
    cout << "The longest common substring is: " << sa.longest_substring(pat) << endl;
}
