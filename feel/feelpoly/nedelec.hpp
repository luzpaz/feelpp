/* -*- mode: c++; coding: utf-8; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4; show-trailing-whitespace: t -*- vim:fenc=utf-8:ft=tcl:et:sw=4:ts=4:sts=4

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
       Date: 2006-01-14

  Copyright (C) 2011 Université de Grenoble 1 (Joseph Fourier)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 3.0 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file nedelec.hpp
   \author Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
   \date 2011-11-25
 */
#ifndef __Nedelec_H
#define __Nedelec_H 1

#include <boost/ptr_container/ptr_vector.hpp>
#include <boost/assign/std/vector.hpp> // for 'operator+=()'
#include <boost/numeric/ublas/vector.hpp>
#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_proxy.hpp>
#include <boost/numeric/ublas/lu.hpp>

#include <boost/assign/list_of.hpp>
#include <boost/assign/std/vector.hpp>

#include <feel/feelcore/feel.hpp>
#include <feel/feelcore/traits.hpp>
#include <feel/feelalg/lu.hpp>

#include <feel/feelmesh/refentity.hpp>
#include <feel/feelmesh/pointset.hpp>


#include <feel/feelpoly/dualbasis.hpp>
#include <feel/feelpoly/polynomialset.hpp>
#include <feel/feelpoly/functionalset.hpp>
#include <feel/feelpoly/operations.hpp>
#include <feel/feelpoly/functionals.hpp>
#include <feel/feelpoly/functionals2.hpp>
#include <feel/feelpoly/quadpoint.hpp>
#include <feel/feelpoly/fe.hpp>

#include <feel/feelvf/vf.hpp>

namespace Feel
{
namespace detail
{
template<typename P>
struct times_rotx
{
    typedef typename P::value_type value_type;
    typedef typename P::points_type points_type;
    times_rotx ( P const& p, int c  )
        :
        _M_p ( p ),
        _M_c( c )
    {
        //std::cout << "component : " << c << std::endl;
    }
    typename ublas::vector<value_type> operator() ( points_type const& __pts ) const
    {
#if 0
        std::cout << "times_x(pts) : " << __pts << std::endl;
        std::cout << "times_x(pts) : " << _M_p.evaluate(__pts) << std::endl;
        std::cout << "times_x(coeff) : " << _M_p.coefficients() << std::endl;
#endif
        // __pts[c] * p( __pts )
        if ( _M_c == 0 )
            return ublas::element_prod( ublas::row( __pts, 1 ),
                                        ublas::row( _M_p.evaluate( __pts ), 0 ) );
        // c == 1
        return ublas::element_prod( -ublas::row( __pts, 0 ),
                                    ublas::row( _M_p.evaluate( __pts ), 0 ) );
    }
    //P const& _M_p;
    P _M_p;
    int _M_c;
};

template< class T >
struct extract_all_poly_indices
{
    T start;
    extract_all_poly_indices( T comp, T N )
        :
        start( comp*N )
    { }

    T operator()()
    {
        return start++;
    }
};
}// detail

template<uint16_type N,
         uint16_type O,
         typename T = double,
         template<uint16_type, uint16_type, uint16_type> class Convex = Simplex>
class NedelecPolynomialSet
    :
        public detail::OrthonormalPolynomialSet<N, O+1, N, Vectorial, T, Convex>
{
    typedef detail::OrthonormalPolynomialSet<N, O+1, N, Vectorial, T, Convex> super;

public:
    typedef detail::OrthonormalPolynomialSet<N, O, N, Vectorial, T, Convex> Pk_v_type;
    typedef detail::OrthonormalPolynomialSet<N, O+1, N, Vectorial, T, Convex> Pkp1_v_type;
    typedef detail::OrthonormalPolynomialSet<N, O-1, N, Vectorial, T, Convex> Pkm1_v_type;
    typedef detail::OrthonormalPolynomialSet<N, O, N, Scalar, T, Convex> Pk_s_type;
    typedef detail::OrthonormalPolynomialSet<N, O+1, N, Scalar, T, Convex> Pkp1_s_type;

    typedef PolynomialSet<typename super::basis_type,Vectorial> vectorial_polynomialset_type;
    typedef typename vectorial_polynomialset_type::polynomial_type vectorial_polynomial_type;
    typedef PolynomialSet<typename super::basis_type,Scalar> scalar_polynomialset_type;
    typedef typename scalar_polynomialset_type::polynomial_type scalar_polynomial_type;

    typedef NedelecPolynomialSet<N, O, T> self_type;

    typedef typename super::value_type value_type;
    typedef typename super::convex_type convex_type;
    typedef typename super::matrix_type matrix_type;
    typedef typename super::points_type points_type;

    static const uint16_type nDim = super::nDim;
    static const uint16_type nOrder = super::nOrder;
    static const uint16_type nComponents = super::nComponents;
    static const bool is_product = false;
    NedelecPolynomialSet()
        :
        super()
    {
        std::cout << "[NPset] nOrder = " << nOrder << "\n";
        std::cout << "[NPset] O = " << O << "\n";
        uint16_type dim_Pkp1 = convex_type::polyDims( nOrder );
        uint16_type dim_Pk = convex_type::polyDims( nOrder-1 );
        uint16_type dim_Pkm1 = (nOrder==1)?0:convex_type::polyDims( nOrder-2 );
#if 1
        std::cout << "[NPset] dim_Pkp1 = " << dim_Pkp1 << "\n";
        std::cout << "[NPset] dim_Pk   = " << dim_Pk << "\n";
        std::cout << "[NPset] dim_Pkm1 = " << dim_Pkm1 << "\n";
#endif
        // (P_k)^d
        Pkp1_v_type Pkp1_v;
        vectorial_polynomialset_type Pk_v( Pkp1_v.polynomialsUpToDimension( dim_Pk ) );
#if 1
        std::cout << "[NPset] Pk_v =" << Pk_v.coeff() << "\n";
#endif
        // P_k
        Pkp1_s_type Pkp1;
        scalar_polynomialset_type Pk ( Pkp1.polynomialsUpToDimension( dim_Pk ) );
#if 1
        std::cout << "[NPset] Pk =" << Pk.coeff() << "\n";
        std::cout << "[NPset] Pk(0) =" << Pk.polynomial( 0 ).coefficients() << "\n";
#endif

        // x P_k \ P_{k-1}
        IMGeneral<convex_type::nDim, 2*nOrder,value_type> im;
        //std::cout << "[RTPset] im.points() = " << im.points() << std::endl;
        ublas::matrix<value_type> xPkc( nComponents*(dim_Pk-dim_Pkm1),Pk.coeff().size2() );

        //std::cout << "[RTPset] before xPkc = " << xPkc << "\n";
        for( int l = dim_Pkm1, i = 0; l < dim_Pk; ++l, ++i )
            {
                for( int j = 0; j < convex_type::nDim; ++j )
                    {
                        detail::times_rotx<scalar_polynomial_type> xp( Pk.polynomial( l ), j );
                        ublas::row(xPkc,i*nComponents+j)=
                            ublas::row( Feel::project( Pkp1,
                                                       xp,
                                                       im ).coeff(), 0);
                    }
            }


        //std::cout << "[RTPset] after xPkc = " << xPkc << "\n";
        vectorial_polynomialset_type xPk( typename super::basis_type(), xPkc, true );
        //std::cout << "[RTPset] here 1\n";
        // (P_k)^d + x P_k
        //std::cout << "[RTPset] RT Poly coeff = " << unite( Pk_v, xPk ).coeff() << "\n";
        this->setCoefficient( unite( Pk_v, xPk ).coeff(), true );
        //std::cout << "[RTPset] here 2\n";
    }


};

namespace fem
{

namespace detail
{



template<typename Basis,
         template<class, uint16_type, class> class PointSetType>
class NedelecDual
    :
        public DualBasis<Basis>
{
    typedef DualBasis<Basis> super;
public:

    static const uint16_type nDim = super::nDim;
    static const uint16_type nOrder= super::nOrder;

    typedef typename super::primal_space_type primal_space_type;
    typedef typename primal_space_type::value_type value_type;
    typedef typename primal_space_type::points_type points_type;
    typedef typename primal_space_type::matrix_type matrix_type;
    typedef typename primal_space_type::template convex<nDim+nOrder-1>::type convex_type;
    typedef Reference<convex_type, nDim, nDim+nOrder-1, nDim, value_type> reference_convex_type;
    typedef typename reference_convex_type::node_type node_type;

    typedef typename primal_space_type::Pkp1_v_type Pkp1_v_type;
    typedef typename primal_space_type::vectorial_polynomialset_type vectorial_polynomialset_type;

    // point set type associated with the functionals
    typedef PointSetType<convex_type, nOrder, value_type> pointset_type;

    static const uint16_type nbPtsPerVertex = 0;
    static const uint16_type nbPtsPerEdge = mpl::if_<mpl::equal_to<mpl::int_<nDim>,mpl::int_<2> >,mpl::int_<reference_convex_type::nbPtsPerEdge>,mpl::int_<0> >::type::value;
    static const uint16_type nbPtsPerFace =mpl::if_<mpl::equal_to<mpl::int_<nDim>,mpl::int_<3> >,mpl::int_<reference_convex_type::nbPtsPerFace>,mpl::int_<0> >::type::value;
    static const uint16_type nbPtsPerVolume = 0;
    static const uint16_type numPoints = ( reference_convex_type::numGeometricFaces*nbPtsPerFace+reference_convex_type::numEdges*nbPtsPerEdge );

    /** Number of degrees of freedom per vertex */
    static const uint16_type nDofPerVertex = 0;

    /** Number of degrees of freedom per edge */
    static const uint16_type nDofPerEdge = nbPtsPerEdge;

    /** Number of degrees of freedom per face */
    static const uint16_type nDofPerFace = nbPtsPerFace;

    /** Number of degrees  of freedom per volume */
    static const uint16_type nDofPerVolume = 0;

    /** Total number of degrees of freedom (equal to refEle::nDof) */
    static const uint16_type nLocalDof = numPoints;

    NedelecDual( primal_space_type const& primal )
        :
        super( primal ),
        _M_convex_ref(),
        _M_eid(_M_convex_ref.topologicalDimension()+1),
        _M_pts( nDim, numPoints ),
        _M_pts_per_face( convex_type::numTopologicalFaces ),
        _M_fset( primal )
    {
#if 1
        std::cout << "Nedelec finite element(dual): \n";
        std::cout << " o- dim   = " << nDim << "\n";
        std::cout << " o- order = " << nOrder << "\n";
        std::cout << " o- numPoints      = " << numPoints << "\n";
        std::cout << " o- nbPtsPerVertex = " << (int)nbPtsPerVertex << "\n";
        std::cout << " o- nbPtsPerEdge   = " << (int)nbPtsPerEdge << "\n";
        std::cout << " o- nbPtsPerFace   = " << (int)nbPtsPerFace << "\n";
        std::cout << " o- nbPtsPerVolume = " << (int)nbPtsPerVolume << "\n";
        std::cout << " o- nLocalDof      = " << nLocalDof << "\n";
#endif

        // loop on each entity forming the convex of topological
        // dimension nDim-1 ( the faces)
        for ( int p = 0, e = _M_convex_ref.entityRange( nDim-1 ).begin();
              e < _M_convex_ref.entityRange( nDim-1 ).end();
              ++e )
            {
                points_type Gt ( _M_convex_ref.makePoints( nDim-1, e ) );
                _M_pts_per_face[e] =  Gt ;
                if ( Gt.size2() )
                    {
                        //Debug() << "Gt = " << Gt << "\n";
                        //Debug() << "p = " << p << "\n";
                        ublas::subrange( _M_pts, 0, nDim, p, p+Gt.size2() ) = Gt;
                        //for ( size_type j = 0; j < Gt.size2(); ++j )
                        //_M_eid[d].push_back( p+j );
                        p+=Gt.size2();
                    }
            }
        //std::cout << "[RT Dual] done 1\n";
        // compute  \f$ \ell_e( U ) = (U * n[e]) (edge_pts(e)) \f$
        typedef Functional<primal_space_type> functional_type;
        std::vector<functional_type> fset;

        // jacobian of the transformation from reference face to the face in the
        // reference element
        std::vector<double> j;
        {
            // bring 'operator+=()' into scope
            using namespace boost::assign;
            if ( nDim == 2 )
                j += 2.8284271247461903,2.0,2.0;
            if ( nDim == 3 )
                j+= 3.464101615137754, 2, 2, 2;
        }

        //for( int k = 0; k < nDim; ++k )
        {
            // loopover the each edge entities and add the correponding functionals
            for ( int e = _M_convex_ref.entityRange( nDim-1 ).begin();
                  e < _M_convex_ref.entityRange( nDim-1 ).end();
                  ++e )
            {
                typedef Feel::functional::DirectionalComponentPointsEvaluation<primal_space_type> dcpe_type;
                std::cout << "tangent " << e << ":" << _M_convex_ref.tangent(e) << "\n";
                //node_type dir= _M_convex_ref.tangent(e)*j[e];
                node_type dir= _M_convex_ref.tangent(e);

                //dcpe_type __dcpe( primal, 1, dir, pts_per_face[e] );
                dcpe_type __dcpe( primal, dir, _M_pts_per_face[e] );
                std::copy( __dcpe.begin(), __dcpe.end(), std::back_inserter( fset ) );
            }
        }
        //std::cout << "[RT Dual] done 2" << std::endl;
        if ( nOrder-1 > 0 )
            {
                // we need more equations : add interior moment
                // indeed the space is orthogonal to Pk-1
                uint16_type dim_Pkp1 = convex_type::polyDims( nOrder );
                uint16_type dim_Pk = convex_type::polyDims( nOrder-1 );
                uint16_type dim_Pm1 = convex_type::polyDims( nOrder-2 );

                Pkp1_v_type Pkp1;

                vectorial_polynomialset_type Pkm1 ( Pkp1.polynomialsUpToDimension( dim_Pm1 ) );
                //std::cout << "Pkm1 = " << Pkm1.coeff() << "\n";
                //std::cout << "Primal = " << primal.coeff() << "\n";
                for( int i = 0; i < Pkm1.polynomialDimension(); ++i )
                    {
                        typedef functional::IntegralMoment<primal_space_type, vectorial_polynomialset_type> fim_type;
                        //typedef functional::IntegralMoment<Pkp1_v_type, vectorial_polynomialset_type> fim_type;
                        //std::cout << "P(" << i << ")=" << Pkm1.polynomial( i ).coeff() << "\n";
                        fset.push_back( fim_type( primal, Pkm1.polynomial( i ) ) );
                    }
            }
        //std::cout << "[RT Dual] done 3, n fset = " << fset.size() << std::endl;
        _M_fset.setFunctionalSet( fset );
//        std::cout << "[RT DUAL matrix] mat = " << _M_fset.rep() << "\n";
        //std::cout << "[RT Dual] done 4\n";

    }

    /**
     * \return the point set
     */
    //pointset_type const& pointSet() const { return M_pset; }

    points_type const& points() const { return _M_pts; }


    matrix_type operator()( primal_space_type const& pset ) const
    {
        //std::cout << "RT matrix = " << _M_fset( pset ) << std::endl;
        return _M_fset( pset );
    }

    points_type const& points( uint16_type f ) const
    {
        return _M_pts_per_face[f];
    }
    ublas::matrix_column<points_type const> point( uint16_type f, uint32_type __i ) const
    {
        return ublas::column( _M_pts_per_face[f], __i );
    }
    ublas::matrix_column<points_type> point( uint16_type f, uint32_type __i )
    {
        return ublas::column( _M_pts_per_face[f], __i );
    }

private:
    /**
     * set the pointset at face \c f using points \c n
     */
    void setPoints( uint16_type f, points_type const& n )
    {
        _M_pts_per_face[f].resize( n.size1(), n.size2(), false );
        _M_pts_per_face[f] = n;
    }

private:
    reference_convex_type _M_convex_ref;
    std::vector<std::vector<uint16_type> > _M_eid;
    points_type _M_pts;
    std::vector<points_type> _M_pts_per_face;
    FunctionalSet<primal_space_type> _M_fset;


};
}// detail


/**
 * \class Nedelec
 * \brief Nedelec Finite Element
 *
 * \f$ H(div)\f$  conforming element
 *
 * @author Christophe Prud'homme
 */
template<uint16_type N,
         uint16_type O,
         typename T = double,
         template<uint16_type, uint16_type, uint16_type> class Convex = Simplex,
         uint16_type TheTAG = 0 >
class Nedelec
    :
        public FiniteElement<NedelecPolynomialSet<N, O, T, Convex>,
                             detail::NedelecDual,
                             PointSetEquiSpaced >,
        public boost::enable_shared_from_this<Nedelec<N,O,T,Convex> >
{
    typedef FiniteElement<NedelecPolynomialSet<N, O, T, Convex>,
                          detail::NedelecDual,
                          PointSetEquiSpaced > super;
public:

    BOOST_STATIC_ASSERT( N > 1 );

    /** @name Typedefs
     */
    //@{

    static const uint16_type nDim = N;
//static const bool isTransformationEquivalent = false;
    static const bool isTransformationEquivalent = true;
    static const bool isContinuous = true;
    typedef Continuous continuity_type;
    static const uint16_type TAG = TheTAG;

    //static const polynomial_transformation_type transformation = POLYNOMIAL_CONTEXT_NEEDS_1ST_PIOLA_TRANSFORMATION;
    typedef typename super::value_type value_type;
    typedef typename super::primal_space_type primal_space_type;
    typedef typename super::dual_space_type dual_space_type;

    /**
     * Polynomial Set type: scalar or vectorial
     */
    typedef typename super::polyset_type polyset_type;
    static const bool is_vectorial = polyset_type::is_vectorial;
    static const bool is_scalar = polyset_type::is_scalar;
    static const uint16_type nComponents = polyset_type::nComponents;
    static const bool is_product = false;


    typedef typename dual_space_type::convex_type convex_type;
    typedef typename dual_space_type::pointset_type pointset_type;
    typedef typename dual_space_type::reference_convex_type reference_convex_type;
    typedef typename reference_convex_type::node_type node_type;
    typedef typename reference_convex_type::points_type points_type;


    static const uint16_type nOrder =  dual_space_type::nOrder;
    static const uint16_type nbPtsPerVertex = 0;
    static const uint16_type nbPtsPerEdge = mpl::if_<mpl::equal_to<mpl::int_<nDim>,mpl::int_<2> >,
                                                     mpl::int_<reference_convex_type::nbPtsPerEdge>,
                                                     mpl::int_<0> >::type::value;
    static const uint16_type nbPtsPerFace = mpl::if_<mpl::equal_to<mpl::int_<nDim>,mpl::int_<3> >,
                                                     mpl::int_<reference_convex_type::nbPtsPerFace>,
                                                     mpl::int_<0> >::type::value;
    static const uint16_type nbPtsPerVolume = 0;
    static const uint16_type numPoints = ( reference_convex_type::numGeometricFaces*nbPtsPerFace+
                                           reference_convex_type::numEdges*nbPtsPerEdge );

    static const uint16_type nLocalDof = dual_space_type::nLocalDof;
    //@}

    /** @name Constructors, destructor
     */
    //@{

    Nedelec()
        :
        super( dual_space_type( primal_space_type() ) ),
        M_refconvex()
    {
#if 1
        std::cout << "[N] nPtsPerEdge = " << nbPtsPerEdge << "\n";
        std::cout << "[N] nPtsPerFace = " << nbPtsPerFace << "\n";
        std::cout << "[N] numPoints = " << numPoints << "\n";

        std::cout << "[N] nDof = " << super::nDof << "\n";

        std::cout << "[N] coeff : " << this->coeff() << "\n";
        std::cout << "[N] pts : " << this->points() << "\n";
        std::cout << "[N] eval at pts : " << this->evaluate( this->points() ) << "\n";
        std::cout << "[N] is_product : " << is_product << "\n";
#endif
    }
    Nedelec( Nedelec const & cr )
        :
        super( cr ),
        M_refconvex()
    {}
    ~Nedelec()
    {}

    //@}

    /** @name Operator overloads
     */
    //@{


    //@}

    /** @name Accessors
     */
    //@{

    /**
     * \return the reference convex associated with the lagrange polynomials
     */
    reference_convex_type const& referenceConvex() const { return M_refconvex; }

    std::string familyName() const { return "nedelec"; }

    //@}

    /** @name  Mutators
     */
    //@{


    //@}

    /** @name  Methods
     */
    //@{

    template<typename ExprType>
    static auto
    isomorphism( ExprType expr ) -> decltype( Feel::vf::detJ()*(trans(Feel::vf::JinvT())*expr)*Feel::vf::Nref() )
    //isomorphism( ExprType& expr ) -> decltype( expr )
        {
            using namespace Feel::vf;
            return detJ()*(trans(JinvT())*expr)*Nref();
            //return expr;
        }
#if 0
    /**
     *
     * \return the value of the expression at the dof
     */
    template<typename ExprType, typename ContextType>
    std::vector<value_type>
    interpolate( boost::shared_ptr<ContextType>& ctx, ExprType & expr )
        {
            using namespace Feel::vf;
            typedef boost::shared_ptr<ContextType> gmc_ptrtype;
            typedef fusion::map<fusion::pair<detail::gmc<0>, gmc_ptrtype> > map_gmc_type;

            std::vector<value_type> v( nLocalDof );

            // First deal with the face dof
            for( int face = 0; face < numTopologicalFaces; ++face )
            {
                // update the geomap at dof on face
                ctx->update( _face=face, _element=ctx->id() );

                map_gmc_type mapgmc( fusion::make_pair<detail::gmc<0> >( ctx ) );
                expr.update( mapgmc, face );

                for(int q = 0; q < nDofPerFace; ++q )
                {
                    int ldof = nDofPerFace*face+i;
                    v[ldof] = expr.evalq(0,0,i);
                }
            }
            // evaluate expr \cdot n  on each face

            // evaluate moments of the expression
        }

    template<typename GMContext, typename PC, typename Phi, typename GPhi, typename HPhi >
    static void transform( boost::shared_ptr<GMContext> gmc,  boost::shared_ptr<PC> const& pc,
                           Phi& phi_t,
                           GPhi& g_phi_t, const bool do_gradient,
                           HPhi& h_phi_t, const bool do_hessian

        )
        {
            transform ( *gmc, *pc, phi_t, g_phi_t, do_gradient, h_phi_t, do_hessian );
        }
    template<typename GMContext, typename PC, typename Phi, typename GPhi, typename HPhi >
    static void transform( GMContext const& gmc,
                           PC const& pc,
                           Phi& phi_t,
                           GPhi& g_phi_t, const bool do_gradient,
                           HPhi& h_phi_t, const bool do_hessian

        )
        {
            //phi_t = phi; return ;
            typename GMContext::gm_type::matrix_type const B = gmc.B( 0 );
            typename GMContext::gm_type::matrix_type const K = gmc.K( 0 );
            typename GMContext::gm_type::matrix_type JB( K/gmc.J(0) );
#if 0
            std::cout << "K= " << gmc.K(0) << "\n";
            std::cout << "B= " << B << "\n";
            std::cout << "J= " << gmc.J(0) << "\n";
            std::cout << "JB= " << JB << "\n";
#endif
            std::fill( phi_t.data(), phi_t.data()+phi_t.num_elements(), value_type(0));
            if ( do_gradient )
            {
                //std::cout << "compute gradient\n";
                std::fill( g_phi_t.data(), g_phi_t.data()+g_phi_t.num_elements(), value_type(0));
            }
            if ( do_hessian )
                std::fill( h_phi_t.data(), h_phi_t.data()+h_phi_t.num_elements(), value_type(0));

            const uint16_type Q = gmc.nPoints();//_M_grad.size2();

            // transform
            for ( uint16_type i = 0; i < nLocalDof; ++i )
            {
                for ( uint16_type l = 0; l < nDim; ++l )
                {

                    for ( uint16_type p = 0; p < nDim; ++p )
                    {
                        for ( uint16_type q = 0; q < Q; ++q )
                        {
                            // \warning : here the transformation depends on the
                            // numbering of the degrees of freedom of the finite
                            // element
                            //phi_t[i][l][0][q] =  pc.phi(i,l,0,q);
                            phi_t[i][l][0][q] += JB( l, p ) * pc.phi(i,p,0,q);
                            //phi_t[i][l][0][q] = gmc.J( 0 ) * B( p, l ) * pc.phi(i,p,0,q);
                            //std::cout << "pc[" << i << "][" << l << "][" << q << "]=" << pc.phi(i,l,0,q) << "\n";
                            //std::cout << "phi_t[" << i << "][" << l << "][" << q << "]=" << phi_t[i][l][0][q] << "\n";
                        }
                    }
                }
                //if ( do_gradient )
                {

                    for ( uint16_type p = 0; p < nDim; ++p )
                    {
                        for ( uint16_type r = 0; r < nDim; ++r )
                        {
                            for ( uint16_type s = 0; s < Q; ++s )
                            {
                                g_phi_t[i][p][r][s] = 0;
                                for ( uint16_type q = 0; q < nDim; ++q )
                                {
                                    g_phi_t[i][p][r][s] += JB( p, q ) * pc.grad(i,q,r,s);
                                    //g_phi_t[i][p][r][s] = pc.grad(i,p,r,s);

                                    //std::cout << "J G[" << i << "][" << q << "][" << r << "][" << s << "=" << JB( p, q ) * pc.grad(i,q,r,s) << "\n";
                                }
                            }
                            //std::cout << "g_phi_t[" << i << "][" << p << "][" << r << "][" << 0 << "=" << g_phi_t[i][p][r][0] << "\n";

                        }
                    }
                }
                if ( do_hessian )
                {
                }
            }
        }
#endif

//@}



protected:
    reference_convex_type M_refconvex;
private:

};
template<uint16_type N,
         uint16_type O,
         typename T,
         template<uint16_type, uint16_type, uint16_type> class Convex,
         uint16_type TheTAG >
const uint16_type Nedelec<N,O,T,Convex,TheTAG>::nDim;
template<uint16_type N,
         uint16_type O,
         typename T,
         template<uint16_type, uint16_type, uint16_type> class Convex,
         uint16_type TheTAG >
const uint16_type Nedelec<N,O,T,Convex,TheTAG>::nOrder;
template<uint16_type N,
         uint16_type O,
         typename T,
         template<uint16_type, uint16_type, uint16_type> class Convex,
         uint16_type TheTAG >
const uint16_type Nedelec<N,O,T,Convex,TheTAG>::nLocalDof;

} // fem
template<uint16_type Order,
         uint16_type TheTAG=0>
class Nedelec
{
public:
    template<uint16_type N,
             uint16_type R = N,
             typename T = double,
             typename Convex = Simplex<N> >
    struct apply
    {
        typedef typename mpl::if_<mpl::bool_<Convex::is_simplex>,
                                  mpl::identity<fem::Nedelec<N,Order,T,Simplex,TheTAG> >,
                                  mpl::identity<fem::Nedelec<N,Order,T,Hypercube,TheTAG> > >::type::type result_type;
        typedef result_type type;
    };

    template<uint16_type TheNewTAG>
    struct ChangeTag
    {
        typedef Nedelec<Order,TheNewTAG> type;
    };

    typedef Lagrange<Order,Scalar> component_basis_type;

    static const uint16_type TAG = TheTAG;
};

} // Feel
#endif /* __Nedelec_H */
