
/*
 * Copyright 2011 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */


#include "../GrBinHashKey.h"
#include "GrGLProgram.h"
#include "GrGLSL.h"
#include "GrGpuGLShaders.h"
#include "../GrGpuVertex.h"
#include "GrNoncopyable.h"
#include "../GrStringBuilder.h"
#include "../GrRandom.h"

#define SKIP_CACHE_CHECK    true
#define GR_UINT32_MAX   static_cast<uint32_t>(-1)

#include "../GrTHashCache.h"

class GrGpuGLShaders::ProgramCache : public ::GrNoncopyable {
private:
    class Entry;

    typedef GrBinHashKey<Entry, GrGLProgram::kProgramKeySize> ProgramHashKey;

    class Entry : public ::GrNoncopyable {
    public:
        Entry() {}
        void copyAndTakeOwnership(Entry& entry) {
            fProgramData.copyAndTakeOwnership(entry.fProgramData);
            fKey = entry.fKey; // ownership transfer
            fLRUStamp = entry.fLRUStamp;
        }

    public:
        int compare(const ProgramHashKey& key) const { return fKey.compare(key); }

    public:
        GrGLProgram::CachedData fProgramData;
        ProgramHashKey          fKey;
        unsigned int            fLRUStamp;
    };

    GrTHashTable<Entry, ProgramHashKey, 8> fHashCache;

    // We may have kMaxEntries+1 shaders in the GL context because
    // we create a new shader before evicting from the cache.
    enum {
        kMaxEntries = 32
    };
    Entry                       fEntries[kMaxEntries];
    int                         fCount;
    unsigned int                fCurrLRUStamp;
    const GrGLContextInfo&      fGL;

public:
    ProgramCache(const GrGLContextInfo& gl)
        : fCount(0)
        , fCurrLRUStamp(0)
        , fGL(gl) {
    }

    ~ProgramCache() {
        for (int i = 0; i < fCount; ++i) {
            GrGpuGLShaders::DeleteProgram(fGL.interface(),
                                          &fEntries[i].fProgramData);
        }
    }

    void abandon() {
        fCount = 0;
    }

    void invalidateViewMatrices() {
        for (int i = 0; i < fCount; ++i) {
            // set to illegal matrix
            fEntries[i].fProgramData.fViewMatrix = GrMatrix::InvalidMatrix();
        }
    }

    GrGLProgram::CachedData* getProgramData(const GrGLProgram& desc) {
        Entry newEntry;
        newEntry.fKey.setKeyData(desc.keyData());
        
        Entry* entry = fHashCache.find(newEntry.fKey);
        if (NULL == entry) {
            if (!desc.genProgram(fGL, &newEntry.fProgramData)) {
                return NULL;
            }
            if (fCount < kMaxEntries) {
                entry = fEntries + fCount;
                ++fCount;
            } else {
                GrAssert(kMaxEntries == fCount);
                entry = fEntries;
                for (int i = 1; i < kMaxEntries; ++i) {
                    if (fEntries[i].fLRUStamp < entry->fLRUStamp) {
                        entry = fEntries + i;
                    }
                }
                fHashCache.remove(entry->fKey, entry);
                GrGpuGLShaders::DeleteProgram(fGL.interface(),
                                              &entry->fProgramData);
            }
            entry->copyAndTakeOwnership(newEntry);
            fHashCache.insert(entry->fKey, entry);
        }

        entry->fLRUStamp = fCurrLRUStamp;
        if (GR_UINT32_MAX == fCurrLRUStamp) {
            // wrap around! just trash our LRU, one time hit.
            for (int i = 0; i < fCount; ++i) {
                fEntries[i].fLRUStamp = 0;
            }
        }
        ++fCurrLRUStamp;
        return &entry->fProgramData;
    }
};

void GrGpuGLShaders::abandonResources(){
    INHERITED::abandonResources();
    fProgramCache->abandon();
    fHWProgramID = 0;
}

void GrGpuGLShaders::DeleteProgram(const GrGLInterface* gl,
                                   CachedData* programData) {
    GR_GL_CALL(gl, DeleteShader(programData->fVShaderID));
    if (programData->fGShaderID) {
        GR_GL_CALL(gl, DeleteShader(programData->fGShaderID));
    }
    GR_GL_CALL(gl, DeleteShader(programData->fFShaderID));
    GR_GL_CALL(gl, DeleteProgram(programData->fProgramID));
    GR_DEBUGCODE(memset(programData, 0, sizeof(*programData));)
}

////////////////////////////////////////////////////////////////////////////////

#define GL_CALL(X) GR_GL_CALL(this->glInterface(), X)

namespace {

// GrRandoms nextU() values have patterns in the low bits
// So using nextU() % array_count might never take some values.
int random_int(GrRandom* r, int count) {
    return (int)(r->nextF() * count);
}

// min is inclusive, max is exclusive
int random_int(GrRandom* r, int min, int max) {
    return (int)(r->nextF() * (max-min)) + min;
}

bool random_bool(GrRandom* r) {
    return r->nextF() > .5f;
}

}

bool GrGpuGLShaders::programUnitTest() {

    GrGLSLGeneration glslGeneration = 
            GrGetGLSLGeneration(this->glBinding(), this->glInterface());
    static const int STAGE_OPTS[] = {
        0,
        StageDesc::kNoPerspective_OptFlagBit,
        StageDesc::kIdentity_CoordMapping
    };
    static const int IN_CONFIG_FLAGS[] = {
        StageDesc::kNone_InConfigFlag,
        StageDesc::kSwapRAndB_InConfigFlag,
        StageDesc::kSwapRAndB_InConfigFlag |
        StageDesc::kMulRGBByAlpha_RoundUp_InConfigFlag,
        StageDesc::kMulRGBByAlpha_RoundDown_InConfigFlag,
        StageDesc::kSmearAlpha_InConfigFlag,
        StageDesc::kSmearRed_InConfigFlag,
    };
    GrGLProgram program;
    ProgramDesc& pdesc = program.fProgramDesc;

    static const int NUM_TESTS = 512;

    GrRandom random;
    for (int t = 0; t < NUM_TESTS; ++t) {

#if 0
        GrPrintf("\nTest Program %d\n-------------\n", t);
        static const int stop = -1;
        if (t == stop) {
            int breakpointhere = 9;
        }
#endif

        pdesc.fVertexLayout = 0;
        pdesc.fEmitsPointSize = random.nextF() > .5f;
        pdesc.fColorInput = random_int(&random, ProgramDesc::kColorInputCnt);
        pdesc.fCoverageInput = random_int(&random, ProgramDesc::kColorInputCnt);

        pdesc.fColorFilterXfermode = random_int(&random, SkXfermode::kCoeffModesCnt);

        pdesc.fFirstCoverageStage = random_int(&random, GrDrawState::kNumStages);

        pdesc.fVertexLayout |= random_bool(&random) ?
                                    GrDrawTarget::kCoverage_VertexLayoutBit :
                                    0;

#if GR_GL_EXPERIMENTAL_GS
        pdesc.fExperimentalGS = this->getCaps().fGeometryShaderSupport &&
                                random_bool(&random);
#endif
        pdesc.fOutputConfig =  random_int(&random, ProgramDesc::kOutputConfigCnt);

        bool edgeAA = random_bool(&random);
        if (edgeAA) {
            bool vertexEdgeAA = random_bool(&random);
            if (vertexEdgeAA) {
                pdesc.fVertexLayout |= GrDrawTarget::kEdge_VertexLayoutBit;
                if (this->getCaps().fShaderDerivativeSupport) {
                    pdesc.fVertexEdgeType = (GrDrawState::VertexEdgeType) random_int(&random, GrDrawState::kVertexEdgeTypeCnt);
                } else {
                    pdesc.fVertexEdgeType = GrDrawState::kHairLine_EdgeType;
                }
                pdesc.fEdgeAANumEdges = 0;
            } else {
                pdesc.fEdgeAANumEdges = random_int(&random, 1, this->getMaxEdges());
                pdesc.fEdgeAAConcave = random_bool(&random);
            }
        } else {
            pdesc.fEdgeAANumEdges = 0;
        }

        pdesc.fColorMatrixEnabled = random_bool(&random);

        if (this->getCaps().fDualSourceBlendingSupport) {
            pdesc.fDualSrcOutput = random_int(&random, ProgramDesc::kDualSrcOutputCnt);
        } else {
            pdesc.fDualSrcOutput = ProgramDesc::kNone_DualSrcOutput;
        }

        for (int s = 0; s < GrDrawState::kNumStages; ++s) {
            // enable the stage?
            if (random_bool(&random)) {
                // use separate tex coords?
                if (random_bool(&random)) {
                    int t = random_int(&random, GrDrawState::kMaxTexCoords);
                    pdesc.fVertexLayout |= StageTexCoordVertexLayoutBit(s, t);
                } else {
                    pdesc.fVertexLayout |= StagePosAsTexCoordVertexLayoutBit(s);
                }
            }
            // use text-formatted verts?
            if (random_bool(&random)) {
                pdesc.fVertexLayout |= kTextFormat_VertexLayoutBit;
            }
            StageDesc& stage = pdesc.fStages[s];
            stage.fOptFlags = STAGE_OPTS[random_int(&random, GR_ARRAY_COUNT(STAGE_OPTS))];
            stage.fInConfigFlags = IN_CONFIG_FLAGS[random_int(&random, GR_ARRAY_COUNT(IN_CONFIG_FLAGS))];
            stage.fCoordMapping =  random_int(&random, StageDesc::kCoordMappingCnt);
            stage.fFetchMode = random_int(&random, StageDesc::kFetchModeCnt);
            // convolution shaders don't work with persp tex matrix
            if (stage.fFetchMode == StageDesc::kConvolution_FetchMode ||
                stage.fFetchMode == StageDesc::kDilate_FetchMode ||
                stage.fFetchMode == StageDesc::kErode_FetchMode) {
                stage.fOptFlags |= StageDesc::kNoPerspective_OptFlagBit;
            }
            stage.setEnabled(VertexUsesStage(s, pdesc.fVertexLayout));
            static const uint32_t kMulByAlphaMask =
                StageDesc::kMulRGBByAlpha_RoundUp_InConfigFlag |
                StageDesc::kMulRGBByAlpha_RoundDown_InConfigFlag;

            switch (stage.fFetchMode) {
                case StageDesc::kSingle_FetchMode:
                    stage.fKernelWidth = 0;
                    break;
                case StageDesc::kConvolution_FetchMode:
                case StageDesc::kDilate_FetchMode:
                case StageDesc::kErode_FetchMode:
                    stage.fKernelWidth = random_int(&random, 2, 8);
                    stage.fInConfigFlags &= ~kMulByAlphaMask;
                    break;
                case StageDesc::k2x2_FetchMode:
                    stage.fKernelWidth = 0;
                    stage.fInConfigFlags &= ~kMulByAlphaMask;
                    break;
            }
        }
        CachedData cachedData;
        if (!program.genProgram(this->glContextInfo(), &cachedData)) {
            return false;
        }
        DeleteProgram(this->glInterface(), &cachedData);
    }
    return true;
}

GrGpuGLShaders::GrGpuGLShaders(const GrGLContextInfo& ctxInfo)
    : GrGpuGL(ctxInfo) {

    // Enable supported shader-related caps
    if (kDesktop_GrGLBinding == this->glBinding()) {
        fCaps.fDualSourceBlendingSupport =
                            this->glVersion() >= GR_GL_VER(3,3) ||
                            this->hasExtension("GL_ARB_blend_func_extended");
        fCaps.fShaderDerivativeSupport = true;
        // we don't support GL_ARB_geometry_shader4, just GL 3.2+ GS
        fCaps.fGeometryShaderSupport = 
                                this->glVersion() >= GR_GL_VER(3,2) &&
                                this->glslGeneration() >= k150_GrGLSLGeneration;
    } else {
        fCaps.fShaderDerivativeSupport =
                            this->hasExtension("GL_OES_standard_derivatives");
    }

    GR_GL_GetIntegerv(this->glInterface(),
                      GR_GL_MAX_VERTEX_ATTRIBS,
                      &fMaxVertexAttribs);

    fProgramData = NULL;
    fProgramCache = new ProgramCache(this->glContextInfo());

#if 0
    this->programUnitTest();
#endif
}

GrGpuGLShaders::~GrGpuGLShaders() {

    if (fProgramData && 0 != fHWProgramID) {
        // detach the current program so there is no confusion on OpenGL's part
        // that we want it to be deleted
        SkASSERT(fHWProgramID == fProgramData->fProgramID);
        GL_CALL(UseProgram(0));
    }
    delete fProgramCache;
}

const GrMatrix& GrGpuGLShaders::getHWViewMatrix() {
    GrAssert(fProgramData);

    if (GrGLProgram::kSetAsAttribute == 
        fProgramData->fUniLocations.fViewMatrixUni) {
        return fHWDrawState.getViewMatrix();
    } else {
        return fProgramData->fViewMatrix;
    }
}

void GrGpuGLShaders::recordHWViewMatrix(const GrMatrix& matrix) {
    GrAssert(fProgramData);
    if (GrGLProgram::kSetAsAttribute == 
        fProgramData->fUniLocations.fViewMatrixUni) {
        fHWDrawState.setViewMatrix(matrix);
    } else {
        fProgramData->fViewMatrix = matrix;
    }
}

const GrMatrix& GrGpuGLShaders::getHWSamplerMatrix(int stage) {
    GrAssert(fProgramData);

    if (GrGLProgram::kSetAsAttribute == 
        fProgramData->fUniLocations.fStages[stage].fTextureMatrixUni) {
        return fHWDrawState.getSampler(stage).getMatrix();
    } else {
        return fProgramData->fTextureMatrices[stage];
    }
}

void GrGpuGLShaders::recordHWSamplerMatrix(int stage, const GrMatrix& matrix) {
    GrAssert(fProgramData);
    if (GrGLProgram::kSetAsAttribute == 
        fProgramData->fUniLocations.fStages[stage].fTextureMatrixUni) {
        *fHWDrawState.sampler(stage)->matrix() = matrix;
    } else {
        fProgramData->fTextureMatrices[stage] = matrix;
    }
}

void GrGpuGLShaders::onResetContext() {
    INHERITED::onResetContext();

    fHWGeometryState.fVertexOffset = ~0;

    // Third party GL code may have left vertex attributes enabled. Some GL
    // implementations (osmesa) may read vetex attributes that are not required
    // by the current shader. Therefore, we have to ensure that only the
    // attributes we require for the current draw are enabled or we may cause an
    // invalid read.

    // Disable all vertex layout bits so that next flush will assume all
    // optional vertex attributes are disabled.
    fHWGeometryState.fVertexLayout = 0;

    // We always use the this attribute and assume it is always enabled.
    int posAttrIdx = GrGLProgram::PositionAttributeIdx();
    GL_CALL(EnableVertexAttribArray(posAttrIdx));
    // Disable all other vertex attributes.
    for  (int va = 0; va < fMaxVertexAttribs; ++va) {
        if (va != posAttrIdx) {
            GL_CALL(DisableVertexAttribArray(va));
        }
    }

    fHWProgramID = 0;
}

void GrGpuGLShaders::flushViewMatrix() {
    const GrMatrix& vm = this->getDrawState().getViewMatrix();
    if (!GrGpuGLShaders::getHWViewMatrix().cheapEqualTo(vm)) {

        const GrRenderTarget* rt = this->getDrawState().getRenderTarget();
        GrAssert(NULL != rt);
        GrMatrix m;
        m.setAll(
            GrIntToScalar(2) / rt->width(), 0, -GR_Scalar1,
            0,-GrIntToScalar(2) / rt->height(), GR_Scalar1,
            0, 0, GrMatrix::I()[8]);
        m.setConcat(m, vm);

        // ES doesn't allow you to pass true to the transpose param,
        // so do our own transpose
        GrGLfloat mt[]  = {
            GrScalarToFloat(m[GrMatrix::kMScaleX]),
            GrScalarToFloat(m[GrMatrix::kMSkewY]),
            GrScalarToFloat(m[GrMatrix::kMPersp0]),
            GrScalarToFloat(m[GrMatrix::kMSkewX]),
            GrScalarToFloat(m[GrMatrix::kMScaleY]),
            GrScalarToFloat(m[GrMatrix::kMPersp1]),
            GrScalarToFloat(m[GrMatrix::kMTransX]),
            GrScalarToFloat(m[GrMatrix::kMTransY]),
            GrScalarToFloat(m[GrMatrix::kMPersp2])
        };

        if (GrGLProgram::kSetAsAttribute ==  
            fProgramData->fUniLocations.fViewMatrixUni) {
            int baseIdx = GrGLProgram::ViewMatrixAttributeIdx();
            GL_CALL(VertexAttrib4fv(baseIdx + 0, mt+0));
            GL_CALL(VertexAttrib4fv(baseIdx + 1, mt+3));
            GL_CALL(VertexAttrib4fv(baseIdx + 2, mt+6));
        } else {
            GrAssert(GrGLProgram::kUnusedUniform != 
                     fProgramData->fUniLocations.fViewMatrixUni);
            GL_CALL(UniformMatrix3fv(fProgramData->fUniLocations.fViewMatrixUni,
                                     1, false, mt));
        }
        this->recordHWViewMatrix(vm);
    }
}

void GrGpuGLShaders::flushTextureDomain(int s) {
    const GrGLint& uni = fProgramData->fUniLocations.fStages[s].fTexDomUni;
    const GrDrawState& drawState = this->getDrawState();
    if (GrGLProgram::kUnusedUniform != uni) {
        const GrRect &texDom = drawState.getSampler(s).getTextureDomain();

        if (((1 << s) & fDirtyFlags.fTextureChangedMask) ||
            fProgramData->fTextureDomain[s] != texDom) {

            fProgramData->fTextureDomain[s] = texDom;

            float values[4] = {
                GrScalarToFloat(texDom.left()),
                GrScalarToFloat(texDom.top()),
                GrScalarToFloat(texDom.right()),
                GrScalarToFloat(texDom.bottom())
            };

            const GrGLTexture* texture =
                static_cast<const GrGLTexture*>(drawState.getTexture(s));
            GrGLTexture::Orientation orientation = texture->orientation();

            // vertical flip if necessary
            if (GrGLTexture::kBottomUp_Orientation == orientation) {
                values[1] = 1.0f - values[1];
                values[3] = 1.0f - values[3];
                // The top and bottom were just flipped, so correct the ordering
                // of elements so that values = (l, t, r, b).
                SkTSwap(values[1], values[3]);
            }

            GL_CALL(Uniform4fv(uni, 1, values));
        }
    }
}

void GrGpuGLShaders::flushTextureMatrix(int s) {
    const GrGLint& uni = fProgramData->fUniLocations.fStages[s].fTextureMatrixUni;
    const GrDrawState& drawState = this->getDrawState();
    const GrGLTexture* texture =
        static_cast<const GrGLTexture*>(drawState.getTexture(s));
    if (NULL != texture) {
        const GrMatrix& hwMatrix = this->getHWSamplerMatrix(s);
        const GrMatrix& samplerMatrix = drawState.getSampler(s).getMatrix();
        if (GrGLProgram::kUnusedUniform != uni &&
            (((1 << s) & fDirtyFlags.fTextureChangedMask) ||
            !hwMatrix.cheapEqualTo(samplerMatrix))) {

            GrMatrix m = samplerMatrix;
            GrSamplerState::SampleMode mode =
                drawState.getSampler(s).getSampleMode();
            AdjustTextureMatrix(texture, mode, &m);

            // ES doesn't allow you to pass true to the transpose param,
            // so do our own transpose
            GrGLfloat mt[]  = {
                GrScalarToFloat(m[GrMatrix::kMScaleX]),
                GrScalarToFloat(m[GrMatrix::kMSkewY]),
                GrScalarToFloat(m[GrMatrix::kMPersp0]),
                GrScalarToFloat(m[GrMatrix::kMSkewX]),
                GrScalarToFloat(m[GrMatrix::kMScaleY]),
                GrScalarToFloat(m[GrMatrix::kMPersp1]),
                GrScalarToFloat(m[GrMatrix::kMTransX]),
                GrScalarToFloat(m[GrMatrix::kMTransY]),
                GrScalarToFloat(m[GrMatrix::kMPersp2])
            };

            if (GrGLProgram::kSetAsAttribute ==
                fProgramData->fUniLocations.fStages[s].fTextureMatrixUni) {
                int baseIdx = GrGLProgram::TextureMatrixAttributeIdx(s);
                GL_CALL(VertexAttrib4fv(baseIdx + 0, mt+0));
                GL_CALL(VertexAttrib4fv(baseIdx + 1, mt+3));
                GL_CALL(VertexAttrib4fv(baseIdx + 2, mt+6));
            } else {
                GL_CALL(UniformMatrix3fv(uni, 1, false, mt));
            }
            this->recordHWSamplerMatrix(s, drawState.getSampler(s).getMatrix());
        }
    }
}

void GrGpuGLShaders::flushRadial2(int s) {

    const int &uni = fProgramData->fUniLocations.fStages[s].fRadial2Uni;
    const GrSamplerState& sampler = this->getDrawState().getSampler(s);
    if (GrGLProgram::kUnusedUniform != uni &&
        (fProgramData->fRadial2CenterX1[s] != sampler.getRadial2CenterX1() ||
         fProgramData->fRadial2Radius0[s]  != sampler.getRadial2Radius0()  ||
         fProgramData->fRadial2PosRoot[s]  != sampler.isRadial2PosRoot())) {

        GrScalar centerX1 = sampler.getRadial2CenterX1();
        GrScalar radius0 = sampler.getRadial2Radius0();

        GrScalar a = GrMul(centerX1, centerX1) - GR_Scalar1;

        // when were in the degenerate (linear) case the second
        // value will be INF but the program doesn't read it. (We
        // use the same 6 uniforms even though we don't need them
        // all in the linear case just to keep the code complexity
        // down).
        float values[6] = {
            GrScalarToFloat(a),
            1 / (2.f * GrScalarToFloat(a)),
            GrScalarToFloat(centerX1),
            GrScalarToFloat(radius0),
            GrScalarToFloat(GrMul(radius0, radius0)),
            sampler.isRadial2PosRoot() ? 1.f : -1.f
        };
        GL_CALL(Uniform1fv(uni, 6, values));
        fProgramData->fRadial2CenterX1[s] = sampler.getRadial2CenterX1();
        fProgramData->fRadial2Radius0[s]  = sampler.getRadial2Radius0();
        fProgramData->fRadial2PosRoot[s]  = sampler.isRadial2PosRoot();
    }
}

void GrGpuGLShaders::flushConvolution(int s) {
    const GrSamplerState& sampler = this->getDrawState().getSampler(s);
    int kernelUni = fProgramData->fUniLocations.fStages[s].fKernelUni;
    if (GrGLProgram::kUnusedUniform != kernelUni) {
        GL_CALL(Uniform1fv(kernelUni, sampler.getKernelWidth(),
                           sampler.getKernel()));
    }
    int imageIncrementUni = fProgramData->fUniLocations.fStages[s].fImageIncrementUni;
    if (GrGLProgram::kUnusedUniform != imageIncrementUni) {
        const GrGLTexture* texture =
            static_cast<const GrGLTexture*>(this->getDrawState().getTexture(s));
        float imageIncrement[2] = { 0 };
        switch (sampler.getFilterDirection()) {
            case GrSamplerState::kX_FilterDirection:
                imageIncrement[0] = 1.0f / texture->width();
                break;
            case GrSamplerState::kY_FilterDirection:
                imageIncrement[1] = 1.0f / texture->height();
                break;
            default:
                GrCrash("Unknown filter direction.");
        }
        GL_CALL(Uniform2fv(imageIncrementUni, 1, imageIncrement));
    }
}

void GrGpuGLShaders::flushTexelSize(int s) {
    const int& uni = fProgramData->fUniLocations.fStages[s].fNormalizedTexelSizeUni;
    if (GrGLProgram::kUnusedUniform != uni) {
        const GrGLTexture* texture =
            static_cast<const GrGLTexture*>(this->getDrawState().getTexture(s));
        if (texture->width() != fProgramData->fTextureWidth[s] ||
            texture->height() != fProgramData->fTextureHeight[s]) {

            float texelSize[] = {1.f / texture->width(),
                                 1.f / texture->height()};
            GL_CALL(Uniform2fv(uni, 1, texelSize));
            fProgramData->fTextureWidth[s] = texture->width();
            fProgramData->fTextureHeight[s] = texture->height();
        }
    }
}

void GrGpuGLShaders::flushEdgeAAData() {
    const int& uni = fProgramData->fUniLocations.fEdgesUni;
    if (GrGLProgram::kUnusedUniform != uni) {
        int count = this->getDrawState().getNumAAEdges();
        GrDrawState::Edge edges[GrDrawState::kMaxEdges];
        // Flip the edges in Y
        float height = 
            static_cast<float>(this->getDrawState().getRenderTarget()->height());
        for (int i = 0; i < count; ++i) {
            edges[i] = this->getDrawState().getAAEdges()[i];
            float b = edges[i].fY;
            edges[i].fY = -b;
            edges[i].fZ += b * height;
        }
        GL_CALL(Uniform3fv(uni, count, &edges[0].fX));
    }
}

void GrGpuGLShaders::flushColorMatrix() {
    const ProgramDesc& desc = fCurrentProgram.getDesc();
    int matrixUni = fProgramData->fUniLocations.fColorMatrixUni;
    int vecUni = fProgramData->fUniLocations.fColorMatrixVecUni;
    if (GrGLProgram::kUnusedUniform != matrixUni
     && GrGLProgram::kUnusedUniform != vecUni) {
        const float* m = this->getDrawState().getColorMatrix();
        GrGLfloat mt[]  = {
            m[0], m[5], m[10], m[15],
            m[1], m[6], m[11], m[16],
            m[2], m[7], m[12], m[17],
            m[3], m[8], m[13], m[18],
        };
        static float scale = 1.0f / 255.0f;
        GrGLfloat vec[] = {
            m[4] * scale, m[9] * scale, m[14] * scale, m[19] * scale,
        };
        GL_CALL(UniformMatrix4fv(matrixUni, 1, false, mt));
        GL_CALL(Uniform4fv(vecUni, 1, vec));
    }
}

static const float ONE_OVER_255 = 1.f / 255.f;

#define GR_COLOR_TO_VEC4(color) {\
    GrColorUnpackR(color) * ONE_OVER_255,\
    GrColorUnpackG(color) * ONE_OVER_255,\
    GrColorUnpackB(color) * ONE_OVER_255,\
    GrColorUnpackA(color) * ONE_OVER_255 \
}

void GrGpuGLShaders::flushColor(GrColor color) {
    const ProgramDesc& desc = fCurrentProgram.getDesc();
    const GrDrawState& drawState = this->getDrawState();

    if (this->getVertexLayout() & kColor_VertexLayoutBit) {
        // color will be specified per-vertex as an attribute
        // invalidate the const vertex attrib color
        fHWDrawState.setColor(GrColor_ILLEGAL);
    } else {
        switch (desc.fColorInput) {
            case ProgramDesc::kAttribute_ColorInput:
                if (fHWDrawState.getColor() != color) {
                    // OpenGL ES only supports the float varieties of
                    // glVertexAttrib
                    float c[] = GR_COLOR_TO_VEC4(color);
                    GL_CALL(VertexAttrib4fv(GrGLProgram::ColorAttributeIdx(), 
                                            c));
                    fHWDrawState.setColor(color);
                }
                break;
            case ProgramDesc::kUniform_ColorInput:
                if (fProgramData->fColor != color) {
                    // OpenGL ES doesn't support unsigned byte varieties of
                    // glUniform
                    float c[] = GR_COLOR_TO_VEC4(color);
                    GrAssert(GrGLProgram::kUnusedUniform != 
                             fProgramData->fUniLocations.fColorUni);
                    GL_CALL(Uniform4fv(fProgramData->fUniLocations.fColorUni,
                                        1, c));
                    fProgramData->fColor = color;
                }
                break;
            case ProgramDesc::kSolidWhite_ColorInput:
            case ProgramDesc::kTransBlack_ColorInput:
                break;
            default:
                GrCrash("Unknown color type.");
        }
    }
    if (fProgramData->fUniLocations.fColorFilterUni
                != GrGLProgram::kUnusedUniform
            && fProgramData->fColorFilterColor
                != drawState.getColorFilterColor()) {
        float c[] = GR_COLOR_TO_VEC4(drawState.getColorFilterColor());
        GL_CALL(Uniform4fv(fProgramData->fUniLocations.fColorFilterUni, 1, c));
        fProgramData->fColorFilterColor = drawState.getColorFilterColor();
    }
}

void GrGpuGLShaders::flushCoverage(GrColor coverage) {
    const ProgramDesc& desc = fCurrentProgram.getDesc();
    const GrDrawState& drawState = this->getDrawState();


    if (this->getVertexLayout() & kCoverage_VertexLayoutBit) {
        // coverage will be specified per-vertex as an attribute
        // invalidate the const vertex attrib coverage
        fHWDrawState.setCoverage4(GrColor_ILLEGAL);
    } else {
        switch (desc.fCoverageInput) {
            case ProgramDesc::kAttribute_ColorInput:
                if (fHWDrawState.getCoverage() != coverage) {
                    // OpenGL ES only supports the float varieties of
                    // glVertexAttrib
                    float c[] = GR_COLOR_TO_VEC4(coverage);
                    GL_CALL(VertexAttrib4fv(GrGLProgram::CoverageAttributeIdx(), 
                                            c));
                    fHWDrawState.setCoverage(coverage);
                }
                break;
            case ProgramDesc::kUniform_ColorInput:
                if (fProgramData->fCoverage != coverage) {
                    // OpenGL ES doesn't support unsigned byte varieties of
                    // glUniform
                    float c[] = GR_COLOR_TO_VEC4(coverage);
                    GrAssert(GrGLProgram::kUnusedUniform != 
                             fProgramData->fUniLocations.fCoverageUni);
                    GL_CALL(Uniform4fv(fProgramData->fUniLocations.fCoverageUni,
                                        1, c));
                    fProgramData->fCoverage = coverage;
                }
                break;
            case ProgramDesc::kSolidWhite_ColorInput:
            case ProgramDesc::kTransBlack_ColorInput:
                break;
            default:
                GrCrash("Unknown coverage type.");
        }
    }
}

bool GrGpuGLShaders::flushGraphicsState(GrPrimitiveType type) {
    if (!flushGLStateCommon(type)) {
        return false;
    }

    const GrDrawState& drawState = this->getDrawState();

    if (fDirtyFlags.fRenderTargetChanged) {
        // our coords are in pixel space and the GL matrices map to NDC
        // so if the viewport changed, our matrix is now wrong.
        fHWDrawState.setViewMatrix(GrMatrix::InvalidMatrix());
        // we assume all shader matrices may be wrong after viewport changes
        fProgramCache->invalidateViewMatrices();
    }

    GrBlendCoeff srcCoeff;
    GrBlendCoeff dstCoeff;
    BlendOptFlags blendOpts = this->getBlendOpts(false, &srcCoeff, &dstCoeff);
    if (kSkipDraw_BlendOptFlag & blendOpts) {
        return false;
    }

    this->buildProgram(type, blendOpts, dstCoeff);
    fProgramData = fProgramCache->getProgramData(fCurrentProgram);
    if (NULL == fProgramData) {
        GrAssert(!"Failed to create program!");
        return false;
    }

    if (fHWProgramID != fProgramData->fProgramID) {
        GL_CALL(UseProgram(fProgramData->fProgramID));
        fHWProgramID = fProgramData->fProgramID;
    }
    fCurrentProgram.overrideBlend(&srcCoeff, &dstCoeff);
    this->flushBlend(type, srcCoeff, dstCoeff);

    GrColor color;
    GrColor coverage;
    if (blendOpts & kEmitTransBlack_BlendOptFlag) {
        color = 0;
        coverage = 0;
    } else if (blendOpts & kEmitCoverage_BlendOptFlag) {
        color = 0xffffffff;
        coverage = drawState.getCoverage();
    } else {
        color = drawState.getColor();
        coverage = drawState.getCoverage();
    }
    this->flushColor(color);
    this->flushCoverage(coverage);

    this->flushViewMatrix();

    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        if (this->isStageEnabled(s)) {
            this->flushTextureMatrix(s);

            this->flushRadial2(s);

            this->flushConvolution(s);

            this->flushTexelSize(s);

            this->flushTextureDomain(s);
        }
    }
    this->flushEdgeAAData();
    this->flushColorMatrix();
    resetDirtyFlags();
    return true;
}

void GrGpuGLShaders::postDraw() {
}

void GrGpuGLShaders::setupGeometry(int* startVertex,
                                    int* startIndex,
                                    int vertexCount,
                                    int indexCount) {

    int newColorOffset;
    int newCoverageOffset;
    int newTexCoordOffsets[GrDrawState::kMaxTexCoords];
    int newEdgeOffset;

    GrVertexLayout currLayout = this->getVertexLayout();

    GrGLsizei newStride = VertexSizeAndOffsetsByIdx(
                                            currLayout,
                                            newTexCoordOffsets,
                                            &newColorOffset,
                                            &newCoverageOffset,
                                            &newEdgeOffset);
    int oldColorOffset;
    int oldCoverageOffset;
    int oldTexCoordOffsets[GrDrawState::kMaxTexCoords];
    int oldEdgeOffset;

    GrGLsizei oldStride = VertexSizeAndOffsetsByIdx(
                                            fHWGeometryState.fVertexLayout,
                                            oldTexCoordOffsets,
                                            &oldColorOffset,
                                            &oldCoverageOffset,
                                            &oldEdgeOffset);
    bool indexed = NULL != startIndex;

    int extraVertexOffset;
    int extraIndexOffset;
    this->setBuffers(indexed, &extraVertexOffset, &extraIndexOffset);

    GrGLenum scalarType;
    bool texCoordNorm;
    if (currLayout & kTextFormat_VertexLayoutBit) {
        scalarType = GrGLTextType;
        texCoordNorm = GR_GL_TEXT_TEXTURE_NORMALIZED;
    } else {
        scalarType = GrGLType;
        texCoordNorm = false;
    }

    size_t vertexOffset = (*startVertex + extraVertexOffset) * newStride;
    *startVertex = 0;
    if (indexed) {
        *startIndex += extraIndexOffset;
    }

    // all the Pointers must be set if any of these are true
    bool allOffsetsChange =  fHWGeometryState.fArrayPtrsDirty ||
                             vertexOffset != fHWGeometryState.fVertexOffset ||
                             newStride != oldStride;

    // position and tex coord offsets change if above conditions are true
    // or the type/normalization changed based on text vs nontext type coords.
    bool posAndTexChange = allOffsetsChange ||
                           (((GrGLTextType != GrGLType) || GR_GL_TEXT_TEXTURE_NORMALIZED) &&
                                (kTextFormat_VertexLayoutBit &
                                  (fHWGeometryState.fVertexLayout ^ currLayout)));

    if (posAndTexChange) {
        int idx = GrGLProgram::PositionAttributeIdx();
        GL_CALL(VertexAttribPointer(idx, 2, scalarType, false, newStride, 
                                  (GrGLvoid*)vertexOffset));
        fHWGeometryState.fVertexOffset = vertexOffset;
    }

    for (int t = 0; t < GrDrawState::kMaxTexCoords; ++t) {
        if (newTexCoordOffsets[t] > 0) {
            GrGLvoid* texCoordOffset = (GrGLvoid*)(vertexOffset + newTexCoordOffsets[t]);
            int idx = GrGLProgram::TexCoordAttributeIdx(t);
            if (oldTexCoordOffsets[t] <= 0) {
                GL_CALL(EnableVertexAttribArray(idx));
                GL_CALL(VertexAttribPointer(idx, 2, scalarType, texCoordNorm, 
                                          newStride, texCoordOffset));
            } else if (posAndTexChange ||
                       newTexCoordOffsets[t] != oldTexCoordOffsets[t]) {
                GL_CALL(VertexAttribPointer(idx, 2, scalarType, texCoordNorm, 
                                          newStride, texCoordOffset));
            }
        } else if (oldTexCoordOffsets[t] > 0) {
            GL_CALL(DisableVertexAttribArray(GrGLProgram::TexCoordAttributeIdx(t)));
        }
    }

    if (newColorOffset > 0) {
        GrGLvoid* colorOffset = (int8_t*)(vertexOffset + newColorOffset);
        int idx = GrGLProgram::ColorAttributeIdx();
        if (oldColorOffset <= 0) {
            GL_CALL(EnableVertexAttribArray(idx));
            GL_CALL(VertexAttribPointer(idx, 4, GR_GL_UNSIGNED_BYTE,
                                      true, newStride, colorOffset));
        } else if (allOffsetsChange || newColorOffset != oldColorOffset) {
            GL_CALL(VertexAttribPointer(idx, 4, GR_GL_UNSIGNED_BYTE,
                                      true, newStride, colorOffset));
        }
    } else if (oldColorOffset > 0) {
        GL_CALL(DisableVertexAttribArray(GrGLProgram::ColorAttributeIdx()));
    }

    if (newCoverageOffset > 0) {
        GrGLvoid* coverageOffset = (int8_t*)(vertexOffset + newCoverageOffset);
        int idx = GrGLProgram::CoverageAttributeIdx();
        if (oldCoverageOffset <= 0) {
            GL_CALL(EnableVertexAttribArray(idx));
            GL_CALL(VertexAttribPointer(idx, 4, GR_GL_UNSIGNED_BYTE,
                                        true, newStride, coverageOffset));
        } else if (allOffsetsChange || newCoverageOffset != oldCoverageOffset) {
            GL_CALL(VertexAttribPointer(idx, 4, GR_GL_UNSIGNED_BYTE,
                                        true, newStride, coverageOffset));
        }
    } else if (oldCoverageOffset > 0) {
        GL_CALL(DisableVertexAttribArray(GrGLProgram::CoverageAttributeIdx()));
    }

    if (newEdgeOffset > 0) {
        GrGLvoid* edgeOffset = (int8_t*)(vertexOffset + newEdgeOffset);
        int idx = GrGLProgram::EdgeAttributeIdx();
        if (oldEdgeOffset <= 0) {
            GL_CALL(EnableVertexAttribArray(idx));
            GL_CALL(VertexAttribPointer(idx, 4, scalarType,
                                        false, newStride, edgeOffset));
        } else if (allOffsetsChange || newEdgeOffset != oldEdgeOffset) {
            GL_CALL(VertexAttribPointer(idx, 4, scalarType,
                                        false, newStride, edgeOffset));
        }
    } else if (oldEdgeOffset > 0) {
        GL_CALL(DisableVertexAttribArray(GrGLProgram::EdgeAttributeIdx()));
    }

    fHWGeometryState.fVertexLayout = currLayout;
    fHWGeometryState.fArrayPtrsDirty = false;
}

void GrGpuGLShaders::buildProgram(GrPrimitiveType type,
                                  BlendOptFlags blendOpts,
                                  GrBlendCoeff dstCoeff) {
    ProgramDesc& desc = fCurrentProgram.fProgramDesc;
    const GrDrawState& drawState = this->getDrawState();

    // This should already have been caught
    GrAssert(!(kSkipDraw_BlendOptFlag & blendOpts));

    bool skipCoverage = SkToBool(blendOpts & kEmitTransBlack_BlendOptFlag);

    bool skipColor = SkToBool(blendOpts & (kEmitTransBlack_BlendOptFlag |
                                           kEmitCoverage_BlendOptFlag));

    // The descriptor is used as a cache key. Thus when a field of the
    // descriptor will not affect program generation (because of the vertex
    // layout in use or other descriptor field settings) it should be set
    // to a canonical value to avoid duplicate programs with different keys.

    // Must initialize all fields or cache will have false negatives!
    desc.fVertexLayout = this->getVertexLayout();

    desc.fEmitsPointSize = kPoints_PrimitiveType == type;

    bool requiresAttributeColors = 
        !skipColor && SkToBool(desc.fVertexLayout & kColor_VertexLayoutBit);
    bool requiresAttributeCoverage = 
        !skipCoverage && SkToBool(desc.fVertexLayout &
                                  kCoverage_VertexLayoutBit);

    // fColorInput/fCoverageInput records how colors are specified for the.
    // program. So we strip the bits from the layout to avoid false negatives
    // when searching for an existing program in the cache.
    desc.fVertexLayout &= ~(kColor_VertexLayoutBit | kCoverage_VertexLayoutBit);

    desc.fColorFilterXfermode = skipColor ?
                                SkXfermode::kDst_Mode :
                                drawState.getColorFilterMode();

    desc.fColorMatrixEnabled = drawState.isStateFlagEnabled(GrDrawState::kColorMatrix_StateBit);

    // no reason to do edge aa or look at per-vertex coverage if coverage is
    // ignored
    if (skipCoverage) {
        desc.fVertexLayout &= ~(kEdge_VertexLayoutBit |
                                kCoverage_VertexLayoutBit);
    }

    bool colorIsTransBlack = SkToBool(blendOpts & kEmitTransBlack_BlendOptFlag);
    bool colorIsSolidWhite = (blendOpts & kEmitCoverage_BlendOptFlag) ||
                             (!requiresAttributeColors &&
                              0xffffffff == drawState.getColor());
    if (GR_AGGRESSIVE_SHADER_OPTS && colorIsTransBlack) {
        desc.fColorInput = ProgramDesc::kTransBlack_ColorInput;
    } else if (GR_AGGRESSIVE_SHADER_OPTS && colorIsSolidWhite) {
        desc.fColorInput = ProgramDesc::kSolidWhite_ColorInput;
    } else if (GR_GL_NO_CONSTANT_ATTRIBUTES && !requiresAttributeColors) {
        desc.fColorInput = ProgramDesc::kUniform_ColorInput;
    } else {
        desc.fColorInput = ProgramDesc::kAttribute_ColorInput;
    }
    
    bool covIsSolidWhite = !requiresAttributeCoverage &&
                           0xffffffff == drawState.getCoverage();
    
    if (skipCoverage) {
        desc.fCoverageInput = ProgramDesc::kTransBlack_ColorInput;
    } else if (covIsSolidWhite) {
        desc.fCoverageInput = ProgramDesc::kSolidWhite_ColorInput;
    } else if (GR_GL_NO_CONSTANT_ATTRIBUTES && !requiresAttributeCoverage) {
        desc.fCoverageInput = ProgramDesc::kUniform_ColorInput;
    } else {
        desc.fCoverageInput = ProgramDesc::kAttribute_ColorInput;
    }

    desc.fEdgeAANumEdges = skipCoverage ? 0 : drawState.getNumAAEdges();
    desc.fEdgeAAConcave = desc.fEdgeAANumEdges > 0 &&
                          drawState.isConcaveEdgeAAState();

    int lastEnabledStage = -1;

    if (!skipCoverage && (desc.fVertexLayout &
                          GrDrawTarget::kEdge_VertexLayoutBit)) {
        desc.fVertexEdgeType = drawState.getVertexEdgeType();
    } else {
        // use canonical value when not set to avoid cache misses
        desc.fVertexEdgeType = GrDrawState::kHairLine_EdgeType;
    }

    for (int s = 0; s < GrDrawState::kNumStages; ++s) {
        StageDesc& stage = desc.fStages[s];

        stage.fOptFlags = 0;
        stage.setEnabled(this->isStageEnabled(s));

        bool skip = s < drawState.getFirstCoverageStage() ? skipColor :
                                                             skipCoverage;

        if (!skip && stage.isEnabled()) {
            lastEnabledStage = s;
            const GrGLTexture* texture =
                static_cast<const GrGLTexture*>(drawState.getTexture(s));
            GrAssert(NULL != texture);
            const GrSamplerState& sampler = drawState.getSampler(s);
            // we matrix to invert when orientation is TopDown, so make sure
            // we aren't in that case before flagging as identity.
            if (TextureMatrixIsIdentity(texture, sampler)) {
                stage.fOptFlags |= StageDesc::kIdentityMatrix_OptFlagBit;
            } else if (!sampler.getMatrix().hasPerspective()) {
                stage.fOptFlags |= StageDesc::kNoPerspective_OptFlagBit;
            }
            switch (sampler.getSampleMode()) {
                case GrSamplerState::kNormal_SampleMode:
                    stage.fCoordMapping = StageDesc::kIdentity_CoordMapping;
                    break;
                case GrSamplerState::kRadial_SampleMode:
                    stage.fCoordMapping = StageDesc::kRadialGradient_CoordMapping;
                    break;
                case GrSamplerState::kRadial2_SampleMode:
                    if (sampler.radial2IsDegenerate()) {
                        stage.fCoordMapping =
                            StageDesc::kRadial2GradientDegenerate_CoordMapping;
                    } else {
                        stage.fCoordMapping =
                            StageDesc::kRadial2Gradient_CoordMapping;
                    }
                    break;
                case GrSamplerState::kSweep_SampleMode:
                    stage.fCoordMapping = StageDesc::kSweepGradient_CoordMapping;
                    break;
                default:
                    GrCrash("Unexpected sample mode!");
                    break;
            }

            switch (sampler.getFilter()) {
                // these both can use a regular texture2D()
                case GrSamplerState::kNearest_Filter:
                case GrSamplerState::kBilinear_Filter:
                    stage.fFetchMode = StageDesc::kSingle_FetchMode;
                    break;
                // performs 4 texture2D()s
                case GrSamplerState::k4x4Downsample_Filter:
                    stage.fFetchMode = StageDesc::k2x2_FetchMode;
                    break;
                // performs fKernelWidth texture2D()s
                case GrSamplerState::kConvolution_Filter:
                    stage.fFetchMode = StageDesc::kConvolution_FetchMode;
                    break;
                case GrSamplerState::kDilate_Filter:
                    stage.fFetchMode = StageDesc::kDilate_FetchMode;
                    break;
                case GrSamplerState::kErode_Filter:
                    stage.fFetchMode = StageDesc::kErode_FetchMode;
                    break;
                default:
                    GrCrash("Unexpected filter!");
                    break;
            }

            if (sampler.hasTextureDomain()) {
                GrAssert(GrSamplerState::kClamp_WrapMode ==
                            sampler.getWrapX() &&
                         GrSamplerState::kClamp_WrapMode ==
                            sampler.getWrapY());
                stage.fOptFlags |= StageDesc::kCustomTextureDomain_OptFlagBit;
            }

            stage.fInConfigFlags = 0;
            if (!this->glCaps().textureSwizzleSupport()) {
                if (GrPixelConfigIsAlphaOnly(texture->config())) {
                    // if we don't have texture swizzle support then
                    // the shader must smear the single channel after
                    // reading the texture
                    if (this->glCaps().textureRedSupport()) {
                        // we can use R8 textures so use kSmearRed
                        stage.fInConfigFlags |= 
                                        StageDesc::kSmearRed_InConfigFlag;
                    } else {
                        // we can use A8 textures so use kSmearAlpha
                        stage.fInConfigFlags |= 
                                        StageDesc::kSmearAlpha_InConfigFlag;
                    }
                } else if (sampler.swapsRAndB()) {
                    stage.fInConfigFlags |= StageDesc::kSwapRAndB_InConfigFlag;
                }
            }
            if (GrPixelConfigIsUnpremultiplied(texture->config())) {
                // The shader generator assumes that color channels are bytes
                // when rounding.
                GrAssert(4 == GrBytesPerPixel(texture->config()));
                if (kUpOnWrite_DownOnRead_UnpremulConversion ==
                    fUnpremulConversion) {
                    stage.fInConfigFlags |=
                        StageDesc::kMulRGBByAlpha_RoundDown_InConfigFlag;
                } else {
                    stage.fInConfigFlags |=
                        StageDesc::kMulRGBByAlpha_RoundUp_InConfigFlag;
                }
            }

            if (sampler.getFilter() == GrSamplerState::kConvolution_Filter ||
                sampler.getFilter() == GrSamplerState::kDilate_Filter ||
                sampler.getFilter() == GrSamplerState::kErode_Filter) {
                stage.fKernelWidth = sampler.getKernelWidth();
            } else {
                stage.fKernelWidth = 0;
            }
        } else {
            stage.fOptFlags         = 0;
            stage.fCoordMapping     = (StageDesc::CoordMapping) 0;
            stage.fInConfigFlags    = 0;
            stage.fFetchMode        = (StageDesc::FetchMode) 0;
            stage.fKernelWidth      = 0;
        }
    }

    if (GrPixelConfigIsUnpremultiplied(drawState.getRenderTarget()->config())) {
        // The shader generator assumes that color channels are bytes
        // when rounding.
        GrAssert(4 == GrBytesPerPixel(drawState.getRenderTarget()->config()));
        if (kUpOnWrite_DownOnRead_UnpremulConversion == fUnpremulConversion) {
            desc.fOutputConfig =
                ProgramDesc::kUnpremultiplied_RoundUp_OutputConfig;
        } else {
            desc.fOutputConfig =
                ProgramDesc::kUnpremultiplied_RoundDown_OutputConfig;
        }
    } else {
        desc.fOutputConfig = ProgramDesc::kPremultiplied_OutputConfig;
    }

    desc.fDualSrcOutput = ProgramDesc::kNone_DualSrcOutput;

    // currently the experimental GS will only work with triangle prims
    // (and it doesn't do anything other than pass through values from
    // the VS to the FS anyway).
#if 0 && GR_GL_EXPERIMENTAL_GS
    desc.fExperimentalGS = this->getCaps().fGeometryShaderSupport;
#endif

    // we want to avoid generating programs with different "first cov stage"
    // values when they would compute the same result.
    // We set field in the desc to kNumStages when either there are no 
    // coverage stages or the distinction between coverage and color is
    // immaterial.
    int firstCoverageStage = GrDrawState::kNumStages;
    desc.fFirstCoverageStage = GrDrawState::kNumStages;
    bool hasCoverage = drawState.getFirstCoverageStage() <= lastEnabledStage;
    if (hasCoverage) {
        firstCoverageStage = drawState.getFirstCoverageStage();
    }

    // other coverage inputs
    if (!hasCoverage) {
        hasCoverage =
               desc.fEdgeAANumEdges ||
               requiresAttributeCoverage ||
               (desc.fVertexLayout & GrDrawTarget::kEdge_VertexLayoutBit);
    }

    if (hasCoverage) {
        // color filter is applied between color/coverage computation
        if (SkXfermode::kDst_Mode != desc.fColorFilterXfermode) {
            desc.fFirstCoverageStage = firstCoverageStage;
        }

        if (this->getCaps().fDualSourceBlendingSupport &&
            !(blendOpts & (kEmitCoverage_BlendOptFlag |
                           kCoverageAsAlpha_BlendOptFlag))) {
            if (kZero_BlendCoeff == dstCoeff) {
                // write the coverage value to second color
                desc.fDualSrcOutput =  ProgramDesc::kCoverage_DualSrcOutput;
                desc.fFirstCoverageStage = firstCoverageStage;
            } else if (kSA_BlendCoeff == dstCoeff) {
                // SA dst coeff becomes 1-(1-SA)*coverage when dst is partially 
                // cover
                desc.fDualSrcOutput = ProgramDesc::kCoverageISA_DualSrcOutput;
                desc.fFirstCoverageStage = firstCoverageStage;
            } else if (kSC_BlendCoeff == dstCoeff) {
                // SA dst coeff becomes 1-(1-SA)*coverage when dst is partially
                // cover
                desc.fDualSrcOutput = ProgramDesc::kCoverageISC_DualSrcOutput;
                desc.fFirstCoverageStage = firstCoverageStage;
            }
        }
    }
}
