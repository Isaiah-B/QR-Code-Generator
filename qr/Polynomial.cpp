#include "Polynomial.h"

namespace polynomial
{
    const uint8_t GALOIS_MAX = 255;

    Polynomial MultiplyAlpha(const Polynomial &p1, const Polynomial &p2)
    {
        int resLength = p1.poly.size() + p2.poly.size() - 1;

        Polynomial res(resLength);
        
        for (const Term& p1Term: p1.poly) {
            for (const Term& p2Term : p2.poly) {
                Term resTerm{};
                
                resTerm.alpha = p1Term.alpha + p2Term.alpha;
                if (resTerm.alpha > GALOIS_MAX) {
                    resTerm.alpha %= GALOIS_MAX;
                }
    
                resTerm.exp = p1Term.exp + p2Term.exp;
                if (resTerm.exp > GALOIS_MAX) {
                    resTerm.exp %= GALOIS_MAX;
                }
                
                resTerm.coef = qrData::LOG_ANTILOG_INTEGERS[resTerm.alpha];
    
                size_t idx = res.poly.size() - 1 - resTerm.exp;
                if (res.poly[idx].exp == resTerm.exp) {
                    res.poly[idx] = AddAlpha(res.poly[idx], resTerm);
                }
                else {
                    res.poly[idx] = resTerm;
                }
            }
        }
    
        return res;
    }

    Polynomial CreateGeneratorPoly(Polynomial &prevGenerator)
    {
        size_t prevGeneratorSize = prevGenerator.poly.size();
        int multFactor = static_cast<int>(prevGeneratorSize) - 1;

        Polynomial factorPoly = { { { 0,1 }, { multFactor, 0 } } };

        return MultiplyAlpha(prevGenerator, factorPoly);
    }

    Polynomial CreateMessagePoly(const std::vector<std::byte> &codewords)
    {
        size_t codewordsSize = codewords.size();

        Polynomial res(codewordsSize);

        for (int i{0}; i < codewordsSize; i++) {
            Term t{};
            t.coef = std::to_integer<int>(codewords[i]);
            t.exp = static_cast<int>(codewordsSize) - 1 - i;
            t.alpha = qrData::LOG_ANTILOG_EXPONENTS[t.coef];

            res.poly[i] = t;
        }

        return res;
    }

    Term AddAlpha(Term &t1, Term &t2)
    {
        Term res{};
        res.coef = t1.coef ^ t2.coef;
        res.exp = t1.exp;
        res.alpha = qrData::LOG_ANTILOG_EXPONENTS[res.coef];
    
        return res;
    }

    // Multiply polynomial by a single coefficient of power exp
    void Polynomial::MultiplyExponent(int exp)
    {
        for (Term& polyTerm : poly) {
            polyTerm.exp += exp;
            
            if (polyTerm.exp > GALOIS_MAX)
                polyTerm.exp %= GALOIS_MAX;
        }
    }

    // XOR two polynomials
    Polynomial Xor(const Polynomial &p1, const Polynomial &p2)
    {
        size_t p1Size = p1.poly.size();
        size_t p2Size = p2.poly.size();

        Polynomial longer = p1Size >= p2Size ? p1 : p2;
        Polynomial shorter = p1Size < p2Size ? p1 : p2;

        for (int i{0}; i < shorter.poly.size(); i++) {
            longer.poly[i].coef ^= shorter.poly[i].coef;
            longer.poly[i].alpha = qrData::LOG_ANTILOG_EXPONENTS[longer.poly[i].coef];
        }

        longer.poly.erase(longer.poly.begin());

        return longer;
    }

    // Multiply a polynomial by a single term
    Polynomial MultiplyTerm(const Polynomial &p, const Term &term)
    {
        Polynomial res(p.poly.size());

        for (int i{0}; i < p.poly.size(); i++) {
            Term polyTerm = p.poly[i];

            polyTerm.alpha += term.alpha;
            if (polyTerm.alpha > GALOIS_MAX)
                polyTerm.alpha %= GALOIS_MAX;

            polyTerm.coef = qrData::LOG_ANTILOG_INTEGERS[polyTerm.alpha];

            res.poly[i] = polyTerm;
        }

        return res;
    }
}
