#pragma once

#include <vector>

#include "DataTables.h"

namespace polynomial
{    
    struct Term
    {
        Term() {}
        Term(int _alpha, int _exp)
            : alpha(_alpha), exp(_exp)
        {
            coef = qrData::LOG_ANTILOG_INTEGERS[alpha];
        }
    
        int coef = 1;
        int alpha = 0;
        int exp = -1;
    };
    
    Term AddAlpha(Term& t1, Term& t2);
    
    struct Polynomial
    {
        Polynomial() {}
        Polynomial(size_t size)
        {
            poly = std::vector<Term>(size);
        }
        Polynomial(const std::vector<Term>& polynomial)
            : poly(polynomial)
        {}
    
        std::vector<Term> poly{};

        void MultiplyExponent(int exp);

        Term GetLeadTerm() const { return poly[0]; }
    };

    Polynomial Xor(const Polynomial& p1, const Polynomial& p2);
    Polynomial MultiplyTerm(const Polynomial& p, const Term& term);
    Polynomial MultiplyAlpha(const Polynomial& p1, const Polynomial& p2);
    
    Polynomial CreateGeneratorPoly(Polynomial& prevGenerator);
    Polynomial CreateMessagePoly(const std::vector<std::byte>& codewords);
}