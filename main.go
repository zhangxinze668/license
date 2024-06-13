package main

import (
	"crypto/tls"
	"license/server"
	"log"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
)

func startServer() error {
	r := gin.Default()

	r.POST("/license/sign", server.LicenseSign)

	//gin设置超时时间
	r.Use(func(c *gin.Context) {
		c.Set("deadline", time.Now().Add(5*time.Second))
		c.Next()
	})
	certPEM := `-----BEGIN CERTIFICATE-----
MIIC2jCCAcKgAwIBAgIBATANBgkqhkiG9w0BAQsFADAUMRIwEAYDVQQKEwlaSElQ
VSBBSS4wIBcNMjQwNjEzMDgxMTI5WhgPMjA1NDA2MTMwODExMjlaMBQxEjAQBgNV
BAoTCVpISVBVIEFJLjCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALUY
3ROjU0G0wayIdIsTgEA5Y3azgYs4qPd2Cgu3jQeIBAit1CKSviiHE3CSuc0ngK03
rgMJ6msVFN1RAvCx7tSmyEd5p3DjIP4Q4jc8E+VjpwhAOL0IcjVOA6KTcI/5kYd9
qFcChIZteb4sCv4gKDuvTJtD7taiL9Me27MkfY2vBLF2/2RvCcqe28cqh8FEi3OK
BcBoZhsgRpZA2i3PetaKlZx0u/jNnDKbLYpFky6o9kfplbZH1ca3SKwK9iANABuI
kOTCshOt3JRQb73/L6EblmPnyc1woUGgDHMTikqgEIvVHrfuqBQimEq6yS2jjid9
H89vknawLQlH7InT/18CAwEAAaM1MDMwDgYDVR0PAQH/BAQDAgWgMBMGA1UdJQQM
MAoGCCsGAQUFBwMBMAwGA1UdEwEB/wQCMAAwDQYJKoZIhvcNAQELBQADggEBAKw1
8/i5mh4t5uSEWWnc++4Ln1ewjgBYoidkvf9ohRrWaD1Z0yT9NhMHMeOfP7G0W6US
/INE7DKdKDIZKAsS3JvMUd8bNTUU4paiuRMvcsugyUdVJ4wxEUAzNYO6c1lnwxhv
fJpapX4W12vT62FP8xM0bZbbGlBXJAOGqd3BSTd4RkQ5vPBpTjgxP/yKWwxXDWks
B/eq/+J7cjHPyhf53Dw8DaXYUgeuF2aaHortfXc7V+lnRq4BqQ10O6jGfGg4OMxO
S52KWAFiRAKgYlxaYFziq5/aQqTTFSWbIX/um8scbbkMaghoqy9PMI4twJcyy/SB
dKgftM5U78YErhC4sSg=
-----END CERTIFICATE-----`

	keyPEM := `-----BEGIN RSA PRIVATE KEY-----
MIIEpAIBAAKCAQEAtRjdE6NTQbTBrIh0ixOAQDljdrOBizio93YKC7eNB4gECK3U
IpK+KIcTcJK5zSeArTeuAwnqaxUU3VEC8LHu1KbIR3mncOMg/hDiNzwT5WOnCEA4
vQhyNU4DopNwj/mRh32oVwKEhm15viwK/iAoO69Mm0Pu1qIv0x7bsyR9ja8EsXb/
ZG8Jyp7bxyqHwUSLc4oFwGhmGyBGlkDaLc961oqVnHS7+M2cMpstikWTLqj2R+mV
tkfVxrdIrAr2IA0AG4iQ5MKyE63clFBvvf8voRuWY+fJzXChQaAMcxOKSqAQi9Ue
t+6oFCKYSrrJLaOOJ30fz2+SdrAtCUfsidP/XwIDAQABAoIBAQCxbAWT6sxzsDKa
5SlA5D8fLNpcfespyb4Ii1W2OwLpYQGKuzr9EEVLEWMjRCzSaKQxkD5kbb+DX6kM
VwUJaVmybyACnpZEC9HT+BiYwrw/XY1UkQ7FbFPwE1mOWuLJu1kvpBVcAXRy+yht
/1pZtImWR91GtJx3HXi7Xx0helujhTA7ifXxBqC4wuxLX71wAshm0UFmIHYRZN+/
wtJoI0aWtWnhtE49BJBSN9Zn9UROXBn41weNblkXzjAi3yk24/d2gcEqnVte3dFb
7tiZfPBCZdd4eX2hYnoUcPp0hYNKJ0cfnbGp0s/q47RTk54y3gqh/H1xVyqwCdFv
wwvnIPDRAoGBAMYv0uuK2iosc3H+wDLbTlF5UiM9AlrMrOlBya+8DVuOc5HAHr41
kU50q/CYw5/ie/XrbhNQLGDMECHvzHV12nKLVrE2PtG9nnk0AV9mKfcCmwcXZKDl
Uv94FvXJ+8qQLR5RDgPCuOycfBbvew3i3daYPTXXTcr5BAcGf81Oh2GJAoGBAOns
0UgKinjfY1gWQGBHSkbnu0FLDrGkGxfvk9gonr/sDjmjZjkV4YxAq8klkIazyXti
MzCQ0UhjAJcbjgBJsFtO00wFlVwdbCvTKyRTO85u6WWvKE4jhvQW4ZAR4MN3Ly4m
93CaT1vDGpfMeZbhQbO3RIqQAZx3Goqhj+7eC6enAoGARLZcCHIxxaF3vTQ36uoC
M1k+0xzZ7iU8Zfr7NESc1PuEsinL/uKPqIXm/xnX6V0a4V5o4aVQXZcimMGNP8RG
oqQQwIwwJ4P4p8kSGMNRFWT/3uuLZgVcS1qVi04mUErAedxcYY1nlBGFLGaHYX8g
rsBFqJ8nR5IihVUJhmMNqRECgYATUFU7MHucXT1TkYrBKEGutwVT78GH0zfVcxdu
2nO4/uFxytbu0aqsrM5CLlLPfFqfUE1PrjVJV7s9vq2rwmnIMCrr9O4PC/LZb3b5
adHfSnMEzUBzqyVCU/1Nqtw250lC8z6s0mDI/8PbUqubVH/bOb9z/U3UwFS1q385
xqWJswKBgQDDZvb95y9lM/S4dpAXcOMoUc1dv56SIttFvpQCTfaY7KCKAQYTqEMr
denw6u9kmEerMbAOkP7NyOwm9C9xVyZh9ryLw1wRkoYIsalCWJfJBT7/cHuQTCSS
WMLEBt3nJeWRTyrYSYlGuFiu9ew2lOD5M5GufIUD9ar2Hrvf+g9yLw==
-----END RSA PRIVATE KEY-----`

	cert, err := tls.X509KeyPair([]byte(certPEM), []byte(keyPEM))
	if err != nil {
		log.Fatalf("Failed to load key pair: %v", err)
		return err
	}

	tlsConfig := &tls.Config{
		Certificates: []tls.Certificate{cert},
	}

	var ServerPort = "9999"
	server := &http.Server{
		Addr:         ":" + ServerPort,
		TLSConfig:    tlsConfig,
		Handler:      r,
		ReadTimeout:  5 * time.Second,
		WriteTimeout: 10 * time.Second,
	}

	// err := server.ListenAndServe()
	// if err != nil {
	// 	log.Fatalf("Failed to start server: %v", err)
	// 	return err
	// }
	//开启https服务

	err = server.ListenAndServeTLS("", "")
	if err != nil {
		log.Fatalf("Failed to start server: %v", err)
		return err
	}
	return nil
}

func main() {
	err := startServer()
	if err != nil {
		log.Fatalf("Failed to start server: %v", err)
		return
	}
}

//签发https证书
// func main() {
// 	privacykey, _ := rsa.GenerateKey(rand.Reader, 2048)
// 	template := x509.Certificate{
// 		SerialNumber:          big.NewInt(1),
// 		Subject:               pkix.Name{Organization: []string{"ZHIPU AI."}},
// 		NotBefore:             time.Now(),
// 		NotAfter:              time.Now().AddDate(30, 0, 0),
// 		KeyUsage:              x509.KeyUsageKeyEncipherment | x509.KeyUsageDigitalSignature,
// 		ExtKeyUsage:           []x509.ExtKeyUsage{x509.ExtKeyUsageServerAuth},
// 		BasicConstraintsValid: true,
// 	}
// 	certBytes, _ := x509.CreateCertificate(rand.Reader, &template, &template, &privacykey.PublicKey, privacykey)
// 	certFile, _ := os.Create("cert.pem")
// 	pem.Encode(certFile, &pem.Block{Type: "CERTIFICATE", Bytes: certBytes})
// 	certFile.Close()

// 	keyFile, _ := os.Create("key.pem")
// 	pem.Encode(keyFile, &pem.Block{Type: "RSA PRIVATE KEY", Bytes: x509.MarshalPKCS1PrivateKey(privacykey)})
// 	keyFile.Close()
// }
