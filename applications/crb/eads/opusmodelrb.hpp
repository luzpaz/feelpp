/* -*- mode: c++ -*-

  This file is part of the Feel library

  Author(s): Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
       Date: 2008-12-10

  Copyright (C) 2008 Universit� Joseph Fourier (Grenoble I)

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
   \file opusmodel.hpp
   \author Christophe Prud'homme <christophe.prudhomme@ujf-grenoble.fr>
   \date 2008-12-10
 */
#ifndef __OpusModelRB_H
#define __OpusModelRB_H 1

#include <feel/options.hpp>
#include <feel/feelcore/feel.hpp>
#include <feel/feelpoly/im.hpp>
#include <feel/feelpoly/polynomialset.hpp>
#include <feel/feeldiscr/mesh.hpp>
#include <feel/feeldiscr/functionspace.hpp>
#include <feel/feeldiscr/region.hpp>
#include <feel/feeldiscr/operatorlagrangep1.hpp>

/**/
#include <opusdata.hpp>
#include <opusmodelbase.hpp>
#include <opusdefs.hpp>
#include <eads.hpp>
#include <feel/feelcrb/parameterspace.hpp>
#include <feel/feeldiscr/bdf2.hpp>
/**/
namespace Feel
{
Feel::po::options_description opusModelOptions();

//    template<typename SpaceType> class OpusModelThermalRB;
//template<typename SpaceType> class OpusModelFluidPoiseuille;
//template<typename SpaceType> class OpusModelFluidPoiseuilleRB;
//template<typename SpaceType> class OpusModelFluidOseen;

/*!
  \class OpusModelRB
  \brief Opus ModelRB


  @author Christophe Prud'homme
  @see
*/
template<int OrderU=2, int OrderP=OrderU-1, int OrderT=OrderP>
class OpusModelRB : public OpusModelBase
{
    typedef OpusModelBase super;
public:

    /** @name Typedefs
     */
    //@{
    static const uint16_type Dim = 2;
    static const double kmin = 0.2;
    static const double kmax = 150;

    static const double rmin = 0.1;
    static const double rmax = 100;

    static const double Qmin = 0;
    static const double Qmax = 1000000;
    static const uint16_type ParameterSpaceDimension = 5;

    static const bool is_time_dependent = true;
    //@}
    /** @name Typedefs
     */
    //@{


    typedef double value_type;


    /*mesh*/
    typedef Simplex<Dim> entity_type;
    typedef Mesh<entity_type> mesh_type;
    typedef boost::shared_ptr<mesh_type> mesh_ptrtype;

    typedef Simplex<1,1,2> entity12_type;
    typedef Mesh<entity12_type> mesh12_type;
    typedef boost::shared_ptr<mesh12_type> mesh12_ptrtype;

    typedef Backend<value_type> backend_type;
    typedef boost::shared_ptr<backend_type> backend_ptrtype;

    /*matrix*/
    typedef typename backend_type::sparse_matrix_type sparse_matrix_type;
    typedef typename backend_type::sparse_matrix_ptrtype sparse_matrix_ptrtype;
    typedef typename backend_type::vector_type vector_type;
    typedef typename backend_type::vector_ptrtype vector_ptrtype;

    typedef Eigen::Matrix<double,Eigen::Dynamic,Eigen::Dynamic> eigen_matrix_type;
    typedef eigen_matrix_type ematrix_type;
    typedef boost::shared_ptr<eigen_matrix_type> eigen_matrix_ptrtype;

    /* temperature */
    typedef DiscontinuousInterfaces<fusion::vector<mpl::vector<mpl::int_<3>, mpl::int_<11>, mpl::int_<13> >,mpl::vector<mpl::int_<4>, mpl::int_<11>, mpl::int_<14> > > >  discontinuity_type;
    typedef fusion::vector<Lagrange<OrderT, Scalar, discontinuity_type> > temp_basis_type;
    //typedef fusion::vector<Lagrange<OrderT, Scalar> > temp_basis_type;
    typedef fusion::vector<Lagrange<OrderT, Vectorial> > grad_temp_basis_type;
    typedef Periodic<1,2,value_type> periodic_type;


    typedef temp_basis_type basis_type;

#if defined( OPUS_WITH_THERMAL_DISCONTINUITY )
    typedef FunctionSpace<mesh_type, temp_basis_type, discontinuity_type, periodic_type> temp_functionspace_type;
#else
    typedef FunctionSpace<mesh_type, temp_basis_type, periodic_type> temp_functionspace_type;

#endif
    typedef boost::shared_ptr<temp_functionspace_type> temp_functionspace_ptrtype;
    typedef typename temp_functionspace_type::element_type temp_element_type;
    typedef boost::shared_ptr<temp_element_type> temp_element_ptrtype;

    typedef temp_functionspace_type functionspace_type;
    typedef temp_functionspace_ptrtype functionspace_ptrtype;
    typedef temp_element_ptrtype element_ptrtype;
    typedef temp_element_type element_type;
    typedef temp_functionspace_type space_type;

    typedef FunctionSpace<mesh_type, grad_temp_basis_type> grad_temp_functionspace_type;
    typedef boost::shared_ptr<grad_temp_functionspace_type> grad_temp_functionspace_ptrtype;
    typedef typename grad_temp_functionspace_type::element_type grad_temp_element_type;
    typedef boost::shared_ptr<grad_temp_element_type> grad_temp_element_ptrtype;

    /* 1D profil */
    typedef fusion::vector<Lagrange<1, Scalar> > p1_basis_type;
    typedef FunctionSpace<mesh12_type, p1_basis_type, value_type> p1_functionspace_type;
    typedef boost::shared_ptr<p1_functionspace_type> p1_functionspace_ptrtype;
    typedef typename p1_functionspace_type::element_type p1_element_type;
    typedef boost::shared_ptr<p1_element_type> p1_element_ptrtype;

    /* P0 */
    typedef FunctionSpace<mesh_type, fusion::vector<Lagrange<0, Scalar, Discontinuous > > > p0_space_type;
    typedef boost::shared_ptr<p0_space_type> p0_space_ptrtype;
    typedef typename p0_space_type::element_type p0_element_type;
    typedef boost::shared_ptr<p0_element_type> p0_element_ptrtype;

    //typedef Feel::OpusModelThermalRB<temp_functionspace_type> thermal_operator_type;
    //typedef boost::shared_ptr<thermal_operator_type> thermal_operator_ptrtype;

    /* velocity */
    typedef fusion::vector<Lagrange<OrderU, Vectorial> > velocity_basis_type;
    typedef FunctionSpace<mesh_type, velocity_basis_type, value_type> velocity_functionspace_type;
    typedef boost::shared_ptr<velocity_functionspace_type> velocity_functionspace_ptrtype;
    typedef typename velocity_functionspace_type::element_type velocity_element_type;
    typedef boost::shared_ptr<velocity_element_type> velocity_element_ptrtype;

    /* pressure */
    typedef fusion::vector<Lagrange<OrderP, Scalar> > pressure_basis_type;
    typedef FunctionSpace<mesh_type, pressure_basis_type, value_type> pressure_functionspace_type;
    typedef boost::shared_ptr<pressure_functionspace_type> pressure_functionspace_ptrtype;
    typedef typename pressure_functionspace_type::element_type pressure_element_type;
    typedef boost::shared_ptr<pressure_element_type> pressure_element_ptrtype;
    /* fluid */
    typedef fusion::vector<Lagrange<OrderU, Vectorial>,Lagrange<OrderP, Scalar> > fluid_basis_type;

    typedef FunctionSpace<mesh_type, fluid_basis_type, value_type> fluid_functionspace_type;
    typedef boost::shared_ptr<fluid_functionspace_type> fluid_functionspace_ptrtype;
    typedef typename fluid_functionspace_type::element_type fluid_element_type;
    typedef boost::shared_ptr<fluid_element_type> fluid_element_ptrtype;
    typedef typename fluid_element_type::template sub_element<0>::type fluid_element_0_type;
    typedef typename fluid_element_type::template sub_element<1>::type fluid_element_1_type;


    /* export */
    typedef Exporter<mesh_type> export_type;
    typedef typename Exporter<mesh_type>::timeset_type timeset_type;

    /* parameter space */
    typedef ParameterSpace<ParameterSpaceDimension> parameterspace_type;
    typedef boost::shared_ptr<parameterspace_type> parameterspace_ptrtype;
    typedef typename parameterspace_type::element_type parameter_type;
    typedef typename parameterspace_type::element_ptrtype parameter_ptrtype;
    typedef typename parameterspace_type::sampling_type sampling_type;
    typedef typename parameterspace_type::sampling_ptrtype sampling_ptrtype;


    /* time */
    typedef Bdf<temp_functionspace_type>  temp_bdf_type;
    typedef boost::shared_ptr<temp_bdf_type> temp_bdf_ptrtype;


    typedef Eigen::VectorXd theta_vector_type;
    typedef boost::tuple<theta_vector_type, theta_vector_type, std::vector<theta_vector_type> > theta_vectors_type;

    typedef boost::tuple<std::vector<sparse_matrix_ptrtype>,std::vector<sparse_matrix_ptrtype>, std::vector<std::vector<vector_ptrtype> > > affine_decomposition_type;
    //@}

    /** @name Constructors, destructor
     */
    //@{

    OpusModelRB();
    OpusModelRB( po::variables_map const& vm );
    OpusModelRB( OpusModelRB const & );
    ~OpusModelRB();

    void init();
    void initParametrization();
    //@}

    /** @name Operator overloads
     */
    //@{


    //@}

    /** @name Accessors
     */
    //@{

    // \return the number of terms in affine decomposition of left hand
    // side bilinear form
    int Qa() const;

    // \return the number of terms in affine decomposition of mass matrix
    int Qm() const;


    /**
     * there is at least one output which is the right hand side of the
     * primal problem
     *
     * \return number of outputs associated to the model
     */
    int Nl() const;

    /**
     * \param l the index of output
     * \return number of terms  in affine decomposition of the \p q th output term
     */
    int Ql( int l ) const;

    /**
     * \brief Returns the function space
     */
    functionspace_ptrtype functionSpace() { return M_Th; }

    //! return the parameter space
    parameterspace_ptrtype parameterSpace() const { return M_Dmu;}

    /**
     * \brief compute the theta coefficient for both bilinear and linear form
     * \param mu parameter to evaluate the coefficients
     */
    theta_vectors_type
    computeThetaq( parameter_type const& mu , double time=0 );

    /**
     * \brief return the coefficient vector
     */
    theta_vector_type const& thetaAq() const { return M_thetaAq; }

    /**
     * \brief return the coefficient vector
     */
    theta_vector_type const& thetaMq() const { return M_thetaMq; }


    /**
     * \brief return the coefficient vector
     */
    std::vector<theta_vector_type> const& thetaL() const { return M_thetaL; }

    /**
     * \brief return the coefficient vector \p q component
     *
     */
    value_type thetaAq( int q ) const
        {
            return M_thetaAq( q );
        }

    /**
     * \brief return the coefficient vector \p q component
     *
     */
    value_type thetaMq( int q ) const
        {
            return M_thetaMq( q );
        }

    /**
     * \return the \p q -th term of the \p l -th output
     */
    value_type thetaL( int l, int q ) const
        {
            return M_thetaL[l]( q );
        }

    /**
     * return true if initialized (init() was called), false otherwise
     */
    bool isInitialized() const { return M_is_initialized; }

    //@}

    /** @name  Mutators
     */
    //@{

    /**
     * set the mesh characteristic length to \p s
     */
    void setMeshSize( double s ) { M_meshSize = s; }

    //@}

    /** @name  Methods
     */
    //@{

    /**
     * create a new matrix
     * \return the newly created matrix
     */
    sparse_matrix_ptrtype newMatrix() const;

    /**
     * \brief Returns the affine decomposition
     */
    affine_decomposition_type computeAffineDecomposition()
        {
            return boost::make_tuple(M_Mq, M_Aq, M_L );
        }


    /**
     * \brief solve the model for parameter \p mu
     * \param mu the model parameter
     * \param T the temperature field
     */
    void solve( parameter_type const& mu, element_ptrtype& T );

    /**
     * solve the model for a given parameter \p mu and \p L as right hand side
     * \param transpose if true solve the transposed(dual) problem
     */
    void solve( parameter_type const& mu, element_ptrtype& u, vector_ptrtype const& L, bool transpose = false );

    /**
     * solve for a given parameter \p mu
     */
    void solve( parameter_type const& mu );

    /**
     * solve \f$ M u = f \f$
     */
    void l2solve( vector_ptrtype& u, vector_ptrtype const& f );


    /**
     * update the PDE system with respect to \param mu
     */
    void update( parameter_type const& mu, const double time=0 );

    void run();
    void run( const double * X, unsigned long N, double * Y, unsigned long P );

    //void exportResults( double time, temp_element_type& T, fluid_element_type& U, bool force_export = false );
    void exportResults(double time, temp_element_type& T );


    std::vector<double> sigmaQ( double k,double r, double Q );

    /**
     * returns the scalar product of the boost::shared_ptr vector x and
     * boost::shared_ptr vector y
     */
    double scalarProduct( vector_ptrtype const& X, vector_ptrtype const& Y );

    /**
     * returns the scalar product of the vector x and vector y
     */
    double scalarProduct( vector_type const& x, vector_type const& y );


    /**
     * Given the output index \p output_index and the parameter \p mu, return
     * the value of the corresponding FEM output
     */
    value_type output( int output_index, parameter_type const& mu );


    /**
     * return true if we want to be in a steady state
     */
    bool isSteady(){ return M_is_steady;}

    /**
     * return value of the time step
     */
    double timeStep() { return M_temp_bdf->timeStep();}

    /**
     * return value of the time final
     */
    double timeFinal() { return M_temp_bdf->timeFinal();}

    /**
     * return value of the time initial
     */
    double timeInitial() { return M_temp_bdf->timeInitial();}

    /**
     * return value of the time order
     */
    double timeOrder() { return M_temp_bdf->timeOrder();}

    /**
     * return number of snapshots used
     */
    int computeNumberOfSnapshots() { return M_temp_bdf->timeFinal()/M_temp_bdf->timeStep(); }

    /**
     * return initialization filed used
     */
    double initializationField() { return M_T0;}

    //@}



protected:

private:

private:

    backend_ptrtype backend;
    backend_ptrtype backendM;

    int Nmax;
    int Taille_rb;
    double epsilon1;
    double epsilon2;
    double tau;
    double M_dt;
    bool M_is_initialized;

    double M_meshSize;
    mesh_ptrtype M_mesh;
    mesh_ptrtype M_mesh_air;
    mesh12_ptrtype M_mesh_line;
    mesh12_ptrtype M_mesh_cross_section_2;

    p0_space_ptrtype M_P0h;
    p0_element_ptrtype domains;
    p0_element_ptrtype rhoC;
    p0_element_ptrtype k;
    p0_element_ptrtype Q;
    p1_functionspace_ptrtype M_P1h;
    temp_functionspace_ptrtype M_Th;
    grad_temp_functionspace_ptrtype M_grad_Th;
    //thermal_operator_ptrtype M_thermal;
    element_ptrtype pT,pV;

    boost::shared_ptr<export_type> M_exporter;
    export_type::timeset_ptrtype M_timeSet;

    sparse_matrix_ptrtype D,M;
    std::vector<vector_ptrtype> L;
    std::vector<sparse_matrix_ptrtype> M_Aq;
    std::vector<sparse_matrix_ptrtype> M_Mq;
    std::vector<std::vector<vector_ptrtype> > M_L;
    parameterspace_ptrtype M_Dmu;
    theta_vector_type M_thetaAq;
    theta_vector_type M_thetaMq;
    std::vector<theta_vector_type> M_thetaL;

    temp_bdf_ptrtype M_temp_bdf;
    double M_bdf_coeff;
    element_ptrtype M_bdf_poly;
    double M_T0;//initial value of temperature

    bool M_is_steady;
};
template<int OrderU, int OrderP, int OrderT>
const double
OpusModelRB<OrderU,OrderP,OrderT>::kmin;
template<int OrderU, int OrderP, int OrderT>
const double
OpusModelRB<OrderU,OrderP,OrderT>::kmax;


template<int OrderU, int OrderP, int OrderT>
const double
OpusModelRB<OrderU,OrderP,OrderT>::rmin;
template<int OrderU, int OrderP, int OrderT>
const double
OpusModelRB<OrderU,OrderP,OrderT>::rmax;


template<int OrderU, int OrderP, int OrderT>
const double
OpusModelRB<OrderU,OrderP,OrderT>::Qmin;
template<int OrderU, int OrderP, int OrderT>
const double
OpusModelRB<OrderU,OrderP,OrderT>::Qmax;


typedef OpusModelRB<2,1,2> opusmodel212_type;
}
#endif /* __OpusModelRB_H */