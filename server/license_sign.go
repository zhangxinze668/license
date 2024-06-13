package server

import (
	"context"
	"crypto/md5"
	"encoding/hex"
	"fmt"
	"net/http"
	"os/exec"
	"strconv"
	"strings"

	"github.com/gin-gonic/gin"
	"golang.org/x/sync/semaphore"
	"k8s.io/klog/v2"
)

var sem = semaphore.NewWeighted(10)

func LicenseSign(c *gin.Context) {
	var req LicenseSignRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		klog.Errorf("parameter is not valid: %v", err)
		c.JSON(http.StatusBadRequest, Response{
			Code:      "Invalid Parameter",
			Message:   "Parameter is not valid",
			RequestId: "",
			Data:      "Unmarshal json failed, please check your request body",
		})
		return
	}

	// klog.Infof("Type of req.LicenseEnv: %v", reflect.TypeOf(req.LicenseEnv))
	// klog.Infof("Type of req.LicenseTag: %v", reflect.TypeOf(req.LicenseTag))
	// klog.Infof("Type of req.LicenseDeadline: %v", reflect.TypeOf(req.LicenseDeadline))
	// klog.Infof("Type of req.LicenseEnvMD5: %v", reflect.TypeOf(req.LicenseEnvMD5))

	if strings.Contains(req.LicenseEnv, ",md5:") {
		//计算licenseEnv的md5值
		//",md5:"分割licenseEnv和md5值
		licenseEnvAndMD5 := strings.Split(req.LicenseEnv, ",md5:")
		req.LicenseEnv = licenseEnvAndMD5[0]
		md5Value := licenseEnvAndMD5[1]
		klog.Infof("licenseEnv: %v", req.LicenseEnv)
		klog.Infof("md5Value: %v", md5Value)
		hasher := md5.New()
		hasher.Write([]byte(req.LicenseEnv))
		md5 := hex.EncodeToString(hasher.Sum(nil))
		md5 = md5[:8]
		klog.Infof("md5 value of licenseEnv: %v", md5)
		if md5Value != md5 {
			klog.Errorf("licenseEnvMD5 is not valid: %v", md5)
			c.JSON(http.StatusBadRequest, Response{
				Code:      "Invalid Parameter",
				Message:   "licenseEnvMD5 is not valid",
				RequestId: "",
				Data:      "Please check your licenseEnv, MD5 value is not correct",
			})
			return
		}

	}

	if req.LicenseEnv == "" {
		klog.Errorf("licenseEnv is not valid: %v", req.LicenseEnv)
		c.JSON(http.StatusBadRequest, Response{
			Code:      "InvalidParameter",
			Message:   "licenseEnv is not valid",
			RequestId: "",
			Data:      "Please check your licenseEnv, it should not be empty",
		})
		return
	}

	if req.LicenseTag == "" {
		klog.Errorf("licenseTag is not valid: %v", req.LicenseTag)
		c.JSON(http.StatusBadRequest, Response{
			Code:      "InvalidParameter",
			Message:   "licenseTag is not valid",
			RequestId: "",
			Data:      "Please check your licenseTag, it should not be empty",
		})
		return
	}

	if req.LicenseDeadline == 0 {
		klog.Errorf("licenseDeadline is not valid: %v", req.LicenseDeadline)
		c.JSON(http.StatusBadRequest, Response{
			Code:      "InvalidParameter",
			Message:   "licenseDeadline is not valid",
			RequestId: "",
			Data:      "Please check your licenseDeadline, it should not be 0",
		})
		return
	}

	// Wait for a token from the semaphore
	if err := sem.Acquire(context.Background(), 1); err != nil {
		klog.Errorf("Failed to acquire semaphore: %v", err)
		c.JSON(http.StatusInternalServerError, Response{
			Code:      "InternalError",
			Message:   "Failed to start licenseSign service",
			RequestId: "",
			Data:      nil,
		})
		return
	}

	// Run external program
	//env := req.LicenseEnv + `\0`
	cmd := exec.Command("bash", "-c", fmt.Sprintf("./signTools/license_sign \"%s\" \"%s\" %d", req.LicenseEnv, req.LicenseTag, req.LicenseDeadline))
	sem.Release(1)
	//打印命令
	klog.Infof("Running command with arguments: %v, %v, %v", req.LicenseEnv, req.LicenseTag, strconv.Itoa(req.LicenseDeadline))
	//klog.Infof("Running command: %v", cmd.String())
	output, err := cmd.Output()
	if err != nil {
		klog.Errorf("Failed to run program: %v", err)
		c.JSON(http.StatusInternalServerError, Response{
			Code:      "InternalError",
			Message:   "Failed to start program",
			RequestId: "",
			Data:      nil,
		})
		return
	}

	fmt.Println(string(output))
	c.JSON(http.StatusOK, Response{
		Code:      "Success",
		Message:   "Success to sign license",
		RequestId: "",
		Data:      string(output),
	})

}
