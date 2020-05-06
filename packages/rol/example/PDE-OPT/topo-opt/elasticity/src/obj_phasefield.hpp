// @HEADER
// ************************************************************************
//
//               Rapid Optimization Library (ROL) Package
//                 Copyright (2014) Sandia Corporation
//
// Under terms of Contract DE-AC04-94AL85000, there is a non-exclusive
// license for use of this work by or on behalf of the U.S. Government.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact lead developers:
//              Drew Kouri   (dpkouri@sandia.gov) and
//              Denis Ridzal (dridzal@sandia.gov)
//
// ************************************************************************
// @HEADER

/*! \file  obj.hpp
    \brief Provides the interface for local (cell-based) objective function computations.
*/

#ifndef PDEOPT_QOI_PHASEFIELD_HPP
#define PDEOPT_QOI_PHASEFIELD_HPP

#include "../../../TOOLS/qoi.hpp"
#include "pde_elasticity.hpp"

template <class Real>
class QoI_ModicaMortolaEnergy_PhaseField : public QoI<Real> {
private:
  const ROL::Ptr<FE<Real>> fe_;
  const Real scale_;

public:
  QoI_ModicaMortolaEnergy_PhaseField(const ROL::Ptr<FE<Real>> &fe,
                                     const Real scale = Real(1))
    : fe_(fe), scale_(scale) {}

  Real value(ROL::Ptr<Intrepid::FieldContainer<Real> > & val,
             const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
             const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
             const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    const Real one(1), cPhi(0.75);
    // Get relevant dimensions
    int c = fe_->gradN()->dimension(0);
    int p = fe_->gradN()->dimension(2);
    int d = fe_->gradN()->dimension(3);
    // Initialize output val
    val = ROL::makePtr<Intrepid::FieldContainer<Real>>(c);
    // Evaluate on FE basis
    ROL::Ptr<Intrepid::FieldContainer<Real>> valZ_eval, gradZ_eval, valPhi_eval;
    valZ_eval   = ROL::makePtr<Intrepid::FieldContainer<Real>>(c, p);
    gradZ_eval  = ROL::makePtr<Intrepid::FieldContainer<Real>>(c, p, d);
    valPhi_eval = ROL::makePtr<Intrepid::FieldContainer<Real>>(c, p);
    fe_->evaluateValue(valZ_eval,z_coeff);
    fe_->evaluateGradient(gradZ_eval,z_coeff);
    for (int i = 0; i < c; ++i) {
      for (int j = 0; j < p; ++j) {
        Real zij = (*valZ_eval)(i,j);
        (*valPhi_eval)(i,j) = cPhi*(zij*zij - one)/scale_;
      }
    }
    // Integrate
    fe_->computeIntegral(val,gradZ_eval,gradZ_eval,false);
    fe_->computeIntegral(val,valPhi_eval,valPhi_eval,true);
    Intrepid::RealSpaceTools<Real>::scale(*val, static_cast<Real>(0.5)*scale_);
    return static_cast<Real>(0);
  }

  void gradient_1(ROL::Ptr<Intrepid::FieldContainer<Real> > & grad,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_ModicaMortolaEnergy_PhaseField::gradient_1 is zero.");
  }

  void gradient_2(ROL::Ptr<Intrepid::FieldContainer<Real> > & grad,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    const Real one(1), cPhi(9.0/8.0);
    // Retrieve dimensions.
    int c = fe_->gradN()->dimension(0);
    int f = fe_->gradN()->dimension(1);
    int p = fe_->gradN()->dimension(2);
    int d = fe_->gradN()->dimension(3);
    // Initialize Gradients.
    grad = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,f);
    // Evaluate/interpolate finite element fields on cells.
    ROL::Ptr<Intrepid::FieldContainer<Real>> valZ_eval, gradZ_eval, valPhi_eval;
    valZ_eval   = ROL::makePtr<Intrepid::FieldContainer<Real>>(c, p);
    gradZ_eval  = ROL::makePtr<Intrepid::FieldContainer<Real>>(c, p, d);
    valPhi_eval = ROL::makePtr<Intrepid::FieldContainer<Real>>(c, p);
    fe_->evaluateValue(valZ_eval,z_coeff);
    fe_->evaluateGradient(gradZ_eval,z_coeff);
    for (int i = 0; i < c; ++i) {
      for (int j = 0; j < p; ++j) {
        Real zij = (*valZ_eval)(i,j);
        (*valPhi_eval)(i,j) = cPhi*zij*(zij*zij-one)/scale_;
      }
    }
    // Evaluate gradient of energy.
    Intrepid::FunctionSpaceTools::integrate<Real>(*grad,
                                                  *gradZ_eval,       // grad(Z)
                                                  *fe_->gradNdetJ(), // grad(N)
                                                  Intrepid::COMP_CPP,
                                                  false);
    Intrepid::RealSpaceTools<Real>::scale(*grad,scale_);
    Intrepid::FunctionSpaceTools::integrate<Real>(*grad,
                                                  *valPhi_eval,  // Phi'(Z)
                                                  *fe_->NdetJ(), // N
                                                  Intrepid::COMP_CPP,
                                                  true);
  }

  void HessVec_11(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_ModicaMortolaEnergy_PhaseField::HessVec_11 is zero.");
  }

  void HessVec_12(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_ModicaMortolaEnergy_PhaseField::HessVec_12 is zero.");
  }

  void HessVec_21(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_ModicaMortolaEnergy_PhaseField::HessVec_21 is zero.");
  }

  void HessVec_22(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    const Real one(1), three(3), cPhi(9.0/8.0);
    // Retrieve dimensions.
    int c = fe_->gradN()->dimension(0);
    int f = fe_->gradN()->dimension(1);
    int p = fe_->gradN()->dimension(2);
    int d = fe_->gradN()->dimension(3);
    // Initialize Hessians.
    hess = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,f);
    // Evaluate/interpolate finite element fields on cells.
    ROL::Ptr<Intrepid::FieldContainer<Real>> valZ_eval, valV_eval, gradV_eval, valPhi_eval;
    valZ_eval   = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,p);
    valV_eval   = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,p);
    gradV_eval  = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,p,d);
    valPhi_eval = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,p);
    fe_->evaluateValue(valZ_eval,z_coeff);
    fe_->evaluateValue(valV_eval,v_coeff);
    fe_->evaluateGradient(gradV_eval,v_coeff);
    for (int i = 0; i < c; ++i) {
      for (int j = 0; j < p; ++j) {
        Real zij = (*valZ_eval)(i,j);
        (*valPhi_eval)(i,j) = (*valV_eval)(i,j)*cPhi*(three*zij*zij-one)/scale_;
      }
    }
    // Evaluate hessian-times-vector of energy.
    Intrepid::FunctionSpaceTools::integrate<Real>(*hess,
                                                  *gradV_eval,       // grad(V)
                                                  *fe_->gradNdetJ(), // grad(N)
                                                  Intrepid::COMP_CPP,
                                                  false);
    Intrepid::RealSpaceTools<Real>::scale(*hess,scale_);
    Intrepid::FunctionSpaceTools::integrate<Real>(*hess,
                                                  *valPhi_eval,  // Phi''(Z)V
                                                  *fe_->NdetJ(), // N
                                                  Intrepid::COMP_CPP,
                                                  true);
  }

}; // QoI_ModicaMortolaEnergy_PhaseField

template <class Real>
class QoI_Volume_PhaseField : public QoI<Real> {
private:
  const ROL::Ptr<FE<Real>> fe_;

public:
  QoI_Volume_PhaseField(const ROL::Ptr<FE<Real>> &fe) : fe_(fe) {}

  Real value(ROL::Ptr<Intrepid::FieldContainer<Real> > & val,
             const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
             const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
             const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    // Get relevant dimensions
    int c = fe_->gradN()->dimension(0);
    int p = fe_->gradN()->dimension(2);
    // Initialize output val
    val = ROL::makePtr<Intrepid::FieldContainer<Real>>(c);
    // Evaluate on FE basis
    ROL::Ptr<Intrepid::FieldContainer<Real>> valZ_eval
      = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,p);
    fe_->evaluateValue(valZ_eval,z_coeff);
    // Approximate characteristic function
    for (int i = 0; i < c; ++i) {
      for (int j = 0; j < p; ++j) {
        (*valZ_eval)(i,j) += static_cast<Real>(1);
      }
    }
    // Compute volume
    fe_->computeIntegral(val,valZ_eval,valZ_eval);
    Intrepid::RealSpaceTools<Real>::scale(*val,static_cast<Real>(0.25));
    return static_cast<Real>(0);
  }

  void gradient_1(ROL::Ptr<Intrepid::FieldContainer<Real> > & grad,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_Volume_PhaseField::gradient_1 is zero.");
  }

  void gradient_2(ROL::Ptr<Intrepid::FieldContainer<Real> > & grad,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    // Get relevant dimensions
    int c = fe_->gradN()->dimension(0);
    int f = fe_->gradN()->dimension(1);
    int p = fe_->gradN()->dimension(2);
    int d = fe_->gradN()->dimension(3);
    // Initialize output grad
    grad = ROL::makePtr<Intrepid::FieldContainer<Real>>(c, f);
    // Evaluate on FE basis
    ROL::Ptr<Intrepid::FieldContainer<Real>> valZ_eval
      = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,p);
    fe_->evaluateValue(valZ_eval,z_coeff);
    // Approximate derivative of characteristic function
    for (int i = 0; i < c; ++i) {
      for (int j = 0; j < p; ++j) {
        (*valZ_eval)(i,j) += static_cast<Real>(1);
      }
    }
    // Compute gradient of volume
    Intrepid::FunctionSpaceTools::integrate<Real>(*grad,
                                                  *valZ_eval,
                                                  *(fe_->NdetJ()),
                                                  Intrepid::COMP_CPP, false);
    Intrepid::RealSpaceTools<Real>::scale(*grad,static_cast<Real>(0.5));
  }

  void HessVec_11(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_Volume_PhaseField::HessVec_11 is zero.");
  }

  void HessVec_12(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_Volume_PhaseField::HessVec_12 is zero.");
  }

  void HessVec_21(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    throw Exception::Zero(">>> QoI_Volume_PhaseField::HessVec_21 is zero.");
  }

  void HessVec_22(ROL::Ptr<Intrepid::FieldContainer<Real> > & hess,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & v_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & u_coeff,
                  const ROL::Ptr<const Intrepid::FieldContainer<Real> > & z_coeff = ROL::nullPtr,
                  const ROL::Ptr<const std::vector<Real> > & z_param = ROL::nullPtr) {
    // Get relevant dimensions
    int c = fe_->gradN()->dimension(0);
    int f = fe_->gradN()->dimension(1);
    int p = fe_->gradN()->dimension(2);
    int d = fe_->gradN()->dimension(3);
    // Initialize output grad
    hess = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,f);
    // Evaluate on FE basis
    ROL::Ptr<Intrepid::FieldContainer<Real>> valV_eval
      = ROL::makePtr<Intrepid::FieldContainer<Real>>(c,p);
    fe_->evaluateValue(valV_eval,v_coeff);
    // Compute gradient of volume
    Intrepid::FunctionSpaceTools::integrate<Real>(*hess,
                                                  *valV_eval,
                                                  *(fe_->NdetJ()),
                                                  Intrepid::COMP_CPP, false);
    Intrepid::RealSpaceTools<Real>::scale(*hess,static_cast<Real>(0.5));
  }

}; // QoI_Volume_PhaseField

#endif
