#include <bits/stdc++.h>
using namespace std;

class suffix_automata
{
private:
    struct state
    {
        int len, link;
        map<char, int> next;
    };

    string str;
    int currsize, last;
    vector<state> vec;

    void add_state(char c)
    {
        int cur = currsize++;
        vec[cur].len = vec[last].len + 1;
        int p = last;
        while (p != -1 && !vec[p].next.count(c))
        {
            vec[p].next[c] = cur;
            p = vec[p].link;
        }
        if (p == -1)
        {
            vec[cur].link = 0;
        }
        else
        {
            int q = vec[p].next[c];
            if (vec[p].len + 1 == vec[q].len)
            {
                vec[cur].link = q;
            }
            else
            {
                int clone = currsize++;
                vec[clone].len = vec[p].len + 1;
                vec[clone].next = vec[q].next;
                vec[clone].link = vec[q].link;
                while (p != -1 && vec[p].next[c] == q)
                {
                    vec[p].next[c] = clone;
                    p = vec[p].link;
                }
                vec[q].link = vec[cur].link = clone;
            }
        }
        last = cur;
    }

public:
    suffix_automata(string s) : str(s), currsize(1), last(0)
    {
        vec.resize(2 * s.length());
        vec[0].len = 0;
        vec[0].link = -1;
        for (char c : str)
            add_state(c);
    }

    bool is_substring(string s)
    {
        int curr = 0;
        for (char c : s)
        {
            if (!vec[curr].next.count(c))
                return false;
            curr = vec[curr].next[c];
        }
        return true;
    }

    int longest_substring(string pattern)
    {
        int cur = 0;
        int len = 0;
        int temp = 0;
        string ans;

        for (int i = 0; i < pattern.size(); i++)
        {
            char c = pattern[i];
            if (vec[cur].next.count(c))
            {
                temp++;
                cur = vec[cur].next[c];
            }
            else
            {
                while (~cur and !vec[cur].next.count(c))
                {
                    cur = vec[cur].link;
                }
                if (~cur)
                {
                    temp = vec[cur].len + 1;
                    cur = vec[cur].next[c];
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
        // cout<<ans<<endl;
        return static_cast<int>(ans.size());
    }
};

signed main()
{
    string str;
    cin >> str;
    suffix_automata sa(str);
    while (1)
    {
        cin >> str;
        cout << (sa.is_substring(str) ? "YES" : "NO") << endl;
        cout << sa.longest_substring(str) << endl;
    }
    return 0;
}
