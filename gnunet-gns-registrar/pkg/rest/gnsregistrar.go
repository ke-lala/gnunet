// This file is part of gnsregistrar, a GNS registrar implementation.
// Copyright (C) 2022 Martin Schanzenbach
//
// gnsregistrar is free software: you can redistribute it and/or modify it
// under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License,
// or (at your option) any later version.
//
// gnsregistrar is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
// SPDX-License-Identifier: AGPL3.0-or-later

package gnsregistrar

import (
	"bytes"
	"encoding/base64"
	"encoding/json"
	"errors"
	"fmt"
	"html/template"
	"io"
	"log"
	"net/http"
	"net/url"
	"os"
	"os/exec"
	"regexp"
	"strings"
	"time"

	"github.com/google/uuid"
	"github.com/gorilla/mux"
	"github.com/schanzen/taler-go/pkg/merchant"
	talerutil "github.com/schanzen/taler-go/pkg/util"
	"github.com/skip2/go-qrcode"
	"gopkg.in/ini.v1"
)

type RegistrarConfig struct {
	// The config to use
	Ini *ini.File

	// The Service Version
	Version string

	// The data home location (usually $datadir/gnunet-gns-registrar)
	Datahome string

	// The merchant connection to use
	Merchant merchant.Merchant

	// The loglevel to use
	Loglevel LogLevel
}

// This is metadata stored in the namestore next to a registered zone key.
// It is always stored in a private metadata record and holds registration information such
// as payment status and expiration.
type RegistrationMetadata struct {
	// The expiration in GNS-compatible 64-bit microseconds epoch.
	Expiration uint64 `json:"expiration"`

	// Indication if this registration is already paid and active.
	Paid bool `json:"paid"`

	// The unique order identifier (as received through https://docs.taler.net/core/api-merchant.html).
	OrderID string `json:"order_id"`

	// The unique registration identifier used as token for registration management by customer.
	RegistrationID string `json:"registration_id"`

	// The payment deadline. FIXME this may also be part of the order somehow in the Taler API.
	NeedsPaymentUntil time.Time `json:"needs_payment_until"`
}

// See https://docs.gnunet.org/latest/developers/rest-api/identity.html
type IdentityInfo struct {
	// Base32-encoded GNS zone key
	Pubkey string `json:"pubkey"`

	// The name of the zone/identity
	Name string `json:"name"`
}

// See https://gana.gnunet.org/gnunet-error-codes/gnunet_error_codes.html
type GnunetError struct {
	// Error description
	Description string `json:"error"`

	// Error code
	Code uint32 `json:"error_code"`
}

// See https://docs.gnunet.org/latest/developers/rest-api/namestore.html
type RecordData struct {
	// The string representation of the record data value, e.g. "1.2.3.4" for an A record
	Value string `json:"value"`

	// The string representation of the record type, e.g. "A" for an IPv4 address
	RecordType string `json:"record_type"`

	// The relative expiration time, in microseconds. Set if is_relative_expiration: true
	RelativeExpiration uint64 `json:"relative_expiration"`

	// Whether or not this is a private record
	IsPrivate bool `json:"is_private"`

	// Whether or not the expiration time is relative (else absolute)
	IsRelativeExpiration bool `json:"is_relative_expiration"`

	// Whether or not this is a supplemental record
	IsSupplemental bool `json:"is_supplemental"`

	// Whether or not this is a shadow record
	IsShadow bool `json:"is_shadow"`

	// Whether or not this is a maintenance record
	IsMaintenance bool `json:"is_maintenance"`
}

// See https://docs.gnunet.org/latest/developers/rest-api/namestore.html
type NamestoreRecord struct {
	// Name of the record set
	RecordName string `json:"record_name"`

	// The record set
	Records []RecordData `json:"data"`
}

// Registrar is the primary object of the service
type Registrar struct {

	// The main router
	Router *mux.Router

	// Our configuration from the config.json
	Cfg RegistrarConfig

	// Logger
	Logger *log.Logger

	// Map of supported validators as defined in the configuration
	Validators map[string]bool

	// landing page
	LandingTpl *template.Template

	// name page
	NameTpl *template.Template

	// buy names page
	BuyTpl *template.Template

	// edit registration page
	EditTpl *template.Template

	// Merchant object
	Merchant merchant.Merchant

	// Relative record expiration (NOT registration expiration!)
	RelativeDelegationExpiration time.Duration

	// Registration expiration (NOT record expiration!)
	RelativeRegistrationExpiration time.Duration

	// Registration expiration days count
	RegistrationExpirationDaysCount uint64

	// Payment expiration (time you have to pay for registration)
	PaymentExpiration time.Duration

	// Name of our root zone
	RootZoneName string

	// Key of our root zone
	RootZoneKey string

	// Suggested suffix for our zone
	SuffixHint string

	// Gnunet REST API basename
	GnunetUrl string

	// Gnunet basic auth on/off
	GnunetBasicAuthEnabled bool

	// Gnunet basic auth
	GnunetUsername string

	// Gnunet basic auth
	GnunetPassword string

	// Registrar base URL
	BaseUrl string

	// The template to use for the summary string
	SummaryTemplateString string

	// Valid label regex
	ValidLabelRegex string

	// Valid label script
	ValidLabelScript string

	// Cost for a registration
	RegistrationCost *talerutil.Amount

	// Cost for a registration
	CurrencySpec talerutil.CurrencySpecification
}

type VersionResponse struct {
	// libtool-style representation of the Merchant protocol version, see
	// https://www.gnu.org/software/libtool/manual/html_node/Versioning.html#Versioning
	// The format is "current:revision:age".
	Version string `json:"version"`
}

func (t *Registrar) configResponse(w http.ResponseWriter, r *http.Request) {
	cfg := VersionResponse{
		Version: "0:0:0",
	}
	w.Header().Set("Content-Type", "application/json")
	response, err := json.Marshal(cfg)
	if nil != err {
		t.Logf(LogError, "%s\n", err.Error())
		return
	}
	w.Write(response)
}

func generateRegistrationId() string {
	return uuid.New().String()
}

func (t *Registrar) landingPage(w http.ResponseWriter, r *http.Request) {
	w.Header().Set("Content-Type", "text/html; charset=utf-8")

	fullData := map[string]any{
		"suffixHint": t.SuffixHint,
		"extZkey":    r.URL.Query().Get("zkey"),
		"zoneKey":    t.RootZoneKey,
		"version":    t.Cfg.Version,
		"error":      r.URL.Query().Get("error"),
	}
	err := t.LandingTpl.Execute(w, fullData)
	if err != nil {
		t.Logf(LogError, "%s\n", err.Error())
	}
}

func (t *Registrar) isNameValid(label string) (err error) {
	if label == "@" {
		return fmt.Errorf("'%s' invalid: '@' not allowed", label)
	}
	if strings.Contains(label, ".") {
		return fmt.Errorf("'%s' invalid: '.' not allowed", label)
	}
	if t.ValidLabelRegex != "" {
		matched, _ := regexp.MatchString(t.ValidLabelRegex, label)
		if !matched {
			return fmt.Errorf("label '%s' not allowed by policy", label)
		}
	}
	if t.ValidLabelScript != "" {
		path, err := exec.LookPath(t.ValidLabelScript)
		if err != nil {
			t.Logf(LogError, "%s\n", err.Error())
			return errors.New("internal error")
		}
		_, err = exec.Command(path, label).Output()
		if err != nil {
			return fmt.Errorf("label '%s' not allowed by policy", label)
		}
	}
	return
}

func (t *Registrar) searchPage(w http.ResponseWriter, r *http.Request) {
	var (
		label string
		zkey  string
		err   error
	)
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	label = r.URL.Query().Get("label")
	err = t.isNameValid(label)
	if nil != err {
		http.Redirect(w, r, fmt.Sprintf("/?error=%s", err), http.StatusSeeOther)
		return
	}
	zkey = r.URL.Query().Get("zkey")
	if zkey == "" {
		http.Redirect(w, r, "/name/"+url.QueryEscape(label), http.StatusSeeOther)
	} else {
		http.Redirect(w, r, "/name/"+url.QueryEscape(label)+"?zkey="+url.QueryEscape(zkey), http.StatusSeeOther)
	}
}

func (t *Registrar) expireRegistration(label string) (err error) {
	var (
		gnunetError GnunetError
		client      *http.Client
	)
	client = &http.Client{}
	req, _ := http.NewRequest(http.MethodDelete, t.GnunetUrl+"/namestore/"+t.RootZoneName+"/"+label, nil)
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if err != nil {
		return err
	}
	err = resp.Body.Close()
	if err != nil {
		return err
	}
	if http.StatusNotFound == resp.StatusCode {
		return nil
	}
	if http.StatusNoContent != resp.StatusCode {
		t.Logf(LogError, "Got error: %d\n", resp.StatusCode)
		_ = json.NewDecoder(resp.Body).Decode(&gnunetError)
		return errors.New("GNUnet REST API error: " + gnunetError.Description)
	}
	return nil
}

func (t *Registrar) createOrUpdateRegistration(nsRecord *NamestoreRecord) (err error) {
	var gnunetError GnunetError
	reqString, _ := json.Marshal(nsRecord)
	client := &http.Client{}
	req, _ := http.NewRequest(http.MethodPut, t.GnunetUrl+"/namestore/"+t.RootZoneName, bytes.NewBuffer(reqString))
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if nil != err {
		return err
	}
	err = resp.Body.Close()
	if err != nil {
		return err
	}
	if http.StatusNoContent != resp.StatusCode {
		t.Logf(LogError, "Got error: %d\n", resp.StatusCode)
		err = json.NewDecoder(resp.Body).Decode(&gnunetError)
		if nil != err {
			return errors.New("GNUnet REST API error: " + err.Error())
		}
		return errors.New("GNUnet REST API error: " + gnunetError.Description)
	}
	return nil
}

func getEndOfDay(day time.Time) time.Time {
	return time.Date(day.Year(), day.Month(), day.Day(), 23, 59, 59, 0, day.Location())
}

func (t *Registrar) setupRegistrationMetadataBeforePayment(label string, zkey string, orderId string, paymentUntil time.Time, regId string) (err error) {
	var (
		namestoreRequest     NamestoreRecord
		delegationRecord     RecordData
		metadataRecord       RecordData
		registrationMetadata RegistrationMetadata
	)
	delegationRecord.IsPrivate = true // Private until payment is through
	delegationRecord.IsRelativeExpiration = true
	delegationRecord.IsSupplemental = false
	delegationRecord.IsMaintenance = false
	delegationRecord.IsShadow = false
	delegationRecord.RecordType = guessDelegationRecordType(zkey)
	delegationRecord.RelativeExpiration = uint64(t.RelativeDelegationExpiration.Microseconds())
	delegationRecord.Value = zkey
	metadataRecord.IsPrivate = true
	metadataRecord.IsRelativeExpiration = true
	metadataRecord.IsSupplemental = false
	metadataRecord.IsMaintenance = true
	metadataRecord.IsShadow = false
	metadataRecord.RecordType = "TXT" // FIXME use new recory type
	metadataRecord.RelativeExpiration = uint64(t.RelativeDelegationExpiration.Microseconds())
	registrationMetadata = RegistrationMetadata{
		Paid:              false,
		OrderID:           orderId,
		NeedsPaymentUntil: paymentUntil,
		RegistrationID:    regId,
		Expiration:        uint64(getEndOfDay(time.Now()).UnixMicro()),
	}
	metadataRecordValue, err := json.Marshal(registrationMetadata)
	if nil != err {
		return err
	}
	metadataRecord.Value = string(metadataRecordValue)
	namestoreRequest.RecordName = label
	namestoreRequest.Records = []RecordData{delegationRecord, metadataRecord}
	return t.createOrUpdateRegistration(&namestoreRequest)
}

func (t *Registrar) updateRegistration(w http.ResponseWriter, r *http.Request) {
	var (
		namestoreResponse NamestoreRecord
		zkeyRecord        RecordData
		metaRecord        RecordData
		regMetadata       *RegistrationMetadata
		client            *http.Client
		token             string
		zkey              string
	)
	vars := mux.Vars(r)
	sanitizedLabel := url.QueryEscape(vars["label"])
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	client = &http.Client{}
	req, _ := http.NewRequest(http.MethodGet, t.GnunetUrl+"/namestore/"+t.RootZoneName+"/"+sanitizedLabel+"?include_maintenance=yes", nil)
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if err != nil {
		http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
		t.Logf(LogError, "Failed to get zone contents\n")
		return
	}
	defer resp.Body.Close()
	if http.StatusOK == resp.StatusCode {
		respData, err := io.ReadAll(resp.Body)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		err = json.NewDecoder(bytes.NewReader(respData)).Decode(&namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		regMetadata, err = t.getCurrentRegistrationMetadata(vars["label"], &namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Failed to get registration metadata", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get registration metadata: `%s'\n", err.Error())
			return
		}
	} else if http.StatusNotFound != resp.StatusCode {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Error determining zone status", http.StatusSeeOther)
		return
	}
	if nil == regMetadata {
		http.Redirect(w, r, "/name/"+sanitizedLabel, http.StatusSeeOther)
		return
	}
	err = r.ParseForm()
	if nil != err {
		t.Logf(LogError, "Unable to parse form: `%s'\n", err.Error())
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Form invalid", http.StatusSeeOther)
		return
	}
	token = r.Form.Get("token")
	zkey = r.Form.Get("zkey")
	if regMetadata.RegistrationID != token {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Unauthorized", http.StatusSeeOther)
		return
	}
	for _, record := range namestoreResponse.Records {
		if isDelegationRecordType(record.RecordType) {
			zkeyRecord = record
		} else {
			metaRecord = record
		}
	}
	zkeyRecord.Value = zkey
	zkeyRecord.RecordType = guessDelegationRecordType(zkey)
	namestoreResponse.Records = []RecordData{metaRecord, zkeyRecord}
	err = t.createOrUpdateRegistration(&namestoreResponse)
	if nil != err {
		t.Logf(LogError, "%s\n", err.Error())
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Update: Internal error", http.StatusSeeOther)
		return
	}
	http.Redirect(w, r, "/name/"+sanitizedLabel+"/edit?token="+token, http.StatusSeeOther)
}

func (t *Registrar) editRegistration(w http.ResponseWriter, r *http.Request) {
	var (
		namestoreResponse NamestoreRecord
		regMetadata       *RegistrationMetadata
		value             string
		client            *http.Client
	)
	vars := mux.Vars(r)
	sanitizedLabel := url.QueryEscape(vars["label"])
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	client = &http.Client{}
	req, _ := http.NewRequest(http.MethodGet, t.GnunetUrl+"/namestore/"+t.RootZoneName+"/"+sanitizedLabel+"?include_maintenance=yes", nil)
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if err != nil {
		http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
		t.Logf(LogError, "Failed to get zone contents\n")
		return
	}
	defer resp.Body.Close()
	if http.StatusOK == resp.StatusCode {
		respData, err := io.ReadAll(resp.Body)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		err = json.NewDecoder(bytes.NewReader(respData)).Decode(&namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		regMetadata, err = t.getCurrentRegistrationMetadata(vars["label"], &namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Failed to get registration metadata", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get registration metadata: `%s'\n", err.Error())
			return
		}
	} else if http.StatusNotFound != resp.StatusCode {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Error determining zone status", http.StatusSeeOther)
		return
	}
	if nil == regMetadata {
		http.Redirect(w, r, "/name/"+sanitizedLabel, http.StatusSeeOther)
		return
	}
	if regMetadata.RegistrationID != r.URL.Query().Get("token") {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Unauthorized", http.StatusSeeOther)
		return
	}
	if !regMetadata.Paid {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"/buy/payment?token="+regMetadata.RegistrationID, http.StatusSeeOther)
		return
	}
	for _, record := range namestoreResponse.Records {
		if isDelegationRecordType(record.RecordType) {
			value = record.Value
		}
	}
	registeredUntil := time.UnixMicro(int64(regMetadata.Expiration))
	registeredUntilStr := registeredUntil.Format(time.DateTime)
	remainingDays := int64(time.Until(registeredUntil).Hours() / 24)
	extendedExpiration := time.UnixMicro(int64(regMetadata.Expiration)).Add(t.RelativeRegistrationExpiration).Format(time.DateTime)
	cost, _ := t.RegistrationCost.FormatWithCurrencySpecification(t.CurrencySpec)
	fullData := map[string]any{
		"label":              vars["label"],
		"zkey":               value,
		"extendedExpiration": extendedExpiration,
		"extensionDaysCount": t.RegistrationExpirationDaysCount,
		"remainingDays":      remainingDays,
		"registeredUntil":    registeredUntilStr,
		"token":              r.URL.Query().Get("token"),
		"version":            t.Cfg.Version,
		"error":              r.URL.Query().Get("error"),
		"cost":               cost,
		"suffixHint":         t.SuffixHint,
	}
	err = t.EditTpl.Execute(w, fullData)
	if err != nil {
		t.Logf(LogError, "%s\n", err.Error())
	}
}

func (t *Registrar) paymentPage(w http.ResponseWriter, r *http.Request) {
	var (
		namestoreResponse NamestoreRecord
		regMetadata       *RegistrationMetadata
		client            *http.Client
		label             string
		errorMsg          string
	)
	vars := mux.Vars(r)
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	client = &http.Client{}
	label = vars["label"]
	sanitizedLabel := url.QueryEscape(vars["label"])
	err := t.isNameValid(label)
	if nil != err {
		http.Redirect(w, r, fmt.Sprintf("/?error=%s", err), http.StatusSeeOther)
		return
	}
	req, _ := http.NewRequest(http.MethodGet, t.GnunetUrl+"/namestore/"+t.RootZoneName+"/"+sanitizedLabel+"?include_maintenance=yes", nil)
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if err != nil {
		http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
		t.Logf(LogError, "Failed to get zone contents\n")
		return
	}
	defer resp.Body.Close()
	if http.StatusOK == resp.StatusCode {
		respData, err := io.ReadAll(resp.Body)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		err = json.NewDecoder(bytes.NewReader(respData)).Decode(&namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		regMetadata, err = t.getCurrentRegistrationMetadata(sanitizedLabel, &namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get registration metadata", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get registration metadata: `%s'\n", err.Error())
			return
		}
	} else if http.StatusNotFound != resp.StatusCode {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Payment failed: Error determining zone status", http.StatusSeeOther)
		return
	}
	if nil == regMetadata {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Payment failed: Please try again.", http.StatusSeeOther)
		return
	}
	if regMetadata.Paid {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Payment already paid.", http.StatusSeeOther)
		return
	}
	_, orderStatus, payto, paytoErr := t.Merchant.IsOrderPaid(regMetadata.OrderID)
	if paytoErr != nil {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Payment failed: Error getting payment data", http.StatusSeeOther)
		return
	}
	encodedPng := ""
	if payto != "" {
		qrPng, qrErr := qrcode.Encode(payto, qrcode.Medium, 256)
		if qrErr != nil {
			http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Error generating QR code", http.StatusSeeOther)
			return
		}
		encodedPng = base64.StdEncoding.EncodeToString(qrPng)
	}
	cost, _ := t.RegistrationCost.FormatWithCurrencySpecification(t.CurrencySpec)
	w.Header().Set("Refresh", "20;url="+t.BaseUrl+"/name/"+sanitizedLabel+"/edit?token="+regMetadata.RegistrationID)
	fullData := map[string]interface{}{
		"orderUnpaid":    merchant.OrderUnpaid == orderStatus,
		"qrCode":         template.URL("data:image/png;base64," + encodedPng),
		"payto":          template.URL(payto),
		"fulfillmentUrl": template.URL(t.BaseUrl + "/name/" + sanitizedLabel + "/edit?token=" + regMetadata.RegistrationID),
		"registrationId": regMetadata.RegistrationID,
		"label":          sanitizedLabel,
		"version":        t.Cfg.Version,
		"error":          errorMsg,
		"cost":           cost,
		"suffixHint":     t.SuffixHint,
	}
	err = t.BuyTpl.Execute(w, fullData)
	if err != nil {
		t.Logf(LogError, "%s\n", err.Error())
	}
}

func (t *Registrar) buyPage(w http.ResponseWriter, r *http.Request) {
	var (
		namestoreResponse NamestoreRecord
		regMetadata       *RegistrationMetadata
		client            *http.Client
		label             string
		regId             string
	)
	vars := mux.Vars(r)
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	client = &http.Client{}
	label = vars["label"]
	sanitizedLabel := url.QueryEscape(vars["label"])
	err := t.isNameValid(label)
	if nil != err {
		http.Redirect(w, r, fmt.Sprintf("/?error=%s", err), http.StatusSeeOther)
		return
	}
	req, _ := http.NewRequest(http.MethodGet, t.GnunetUrl+"/namestore/"+t.RootZoneName+"/"+sanitizedLabel+"?include_maintenance=yes", nil)
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if err != nil {
		http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
		t.Logf(LogError, "Failed to get zone contents\n")
		return
	}
	defer resp.Body.Close()
	if http.StatusOK == resp.StatusCode {
		respData, err := io.ReadAll(resp.Body)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		err = json.NewDecoder(bytes.NewReader(respData)).Decode(&namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get zone contents", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			return
		}
		regMetadata, err = t.getCurrentRegistrationMetadata(sanitizedLabel, &namestoreResponse)
		if err != nil {
			http.Redirect(w, r, "/"+"?error=Registration failed: Failed to get registration metadata", http.StatusSeeOther)
			t.Logf(LogError, "Failed to get registration metadata: `%s'\n", err.Error())
			return
		}
	} else if http.StatusNotFound != resp.StatusCode {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Error determining zone status", http.StatusSeeOther)
		return
	}
	if nil != regMetadata {
		if !regMetadata.Paid {
			http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Pending buy order", http.StatusSeeOther)
			return
		}
		regMetadata.Paid = false
		regId = regMetadata.RegistrationID
	} else {
		regId = generateRegistrationId()
	}
	summaryMsg := strings.Replace(t.SummaryTemplateString, "${NAME}", label, 1)
	orderID, newOrderErr := t.Merchant.AddNewOrder(*t.RegistrationCost, summaryMsg, t.BaseUrl+"/name/"+sanitizedLabel+"/edit?token="+regId)
	if newOrderErr != nil {
		t.Logf(LogError, "%s\n", newOrderErr.Error())
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Unable to create order", http.StatusSeeOther)
		return
	}
	// FIXME: based on order status, we probably want to display something else
	_, _, _, paytoErr := t.Merchant.IsOrderPaid(orderID)
	if paytoErr != nil {
		http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Error getting payment data", http.StatusSeeOther)
		return
	}
	paymentUntil := time.Now().Add(t.PaymentExpiration)
	if nil != regMetadata {
		var newZkeyRecord RecordData
		var metaRecord RecordData
		for _, record := range namestoreResponse.Records {
			if isDelegationRecordType(record.RecordType) {
				record.IsPrivate = false
				newZkeyRecord = record
			} else {
				metaRecord = record
			}
		}
		regMetadata.NeedsPaymentUntil = paymentUntil
		regMetadata.OrderID = orderID
		metadataRecordValue, err := json.Marshal(regMetadata)
		if nil != err {
			t.Logf(LogError, "%s\n", err.Error())
			http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Internal error", http.StatusSeeOther)
			return
		}
		metaRecord.Value = string(metadataRecordValue)
		namestoreResponse.Records = []RecordData{metaRecord, newZkeyRecord}
		err = t.createOrUpdateRegistration(&namestoreResponse)
		if nil != err {
			t.Logf(LogError, "%s\n", err.Error())
			http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Internal error", http.StatusSeeOther)
			return
		}
	} else {
		err = t.setupRegistrationMetadataBeforePayment(sanitizedLabel, r.URL.Query().Get("zkey"), orderID, paymentUntil, regId)
		if err != nil {
			t.Logf(LogError, "%s\n", err.Error())
			http.Redirect(w, r, "/name/"+sanitizedLabel+"?error=Registration failed: Internal error", http.StatusSeeOther)
			return
		}
	}
	http.Redirect(w, r, "/name/"+sanitizedLabel+"/buy/payment?token="+regId, http.StatusSeeOther)
}

func (t *Registrar) getCurrentRegistrationMetadata(label string, nsRecord *NamestoreRecord) (*RegistrationMetadata, error) {
	var (
		regMetadata  RegistrationMetadata
		haveMetadata bool
	)
	haveMetadata = false
	for _, record := range nsRecord.Records {
		if record.RecordType == "TXT" {
			err := json.Unmarshal([]byte(record.Value), &regMetadata)
			if err != nil {
				t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
				return nil, err
			}
			haveMetadata = true
		}
	}
	if !haveMetadata {
		return nil, nil
	}
	// Does this registration have an unpaid order? if yes, check payment status and update if necessary.
	if !regMetadata.Paid {
		rc, orderStatus, _, paytoErr := t.Merchant.IsOrderPaid(regMetadata.OrderID)
		if nil != paytoErr {
			if rc == http.StatusNotFound {
				if time.Now().After(time.UnixMicro(int64(regMetadata.Expiration))) {
					t.Logf(LogInfo, "Registration for `%s' not found, removing\n", label)
					err := t.expireRegistration(label)
					if nil != err {
						t.Logf(LogInfo, "%s\n", err.Error())
					}
					return nil, nil
				} else {
					return &regMetadata, nil
				}
			}
			return nil, errors.New("Error determining payment status: " + paytoErr.Error())
		}
		if merchant.OrderPaid == orderStatus {
			// Order was paid!
			regMetadata.Paid = true
			var newZkeyRecord RecordData
			var newMetaRecord RecordData
			for _, record := range nsRecord.Records {
				if isDelegationRecordType(record.RecordType) {
					record.IsPrivate = false
					newZkeyRecord = record
				}
				if record.RecordType == "TXT" {
					metadataRecordValue, err := json.Marshal(regMetadata)
					if nil != err {
						return nil, err
					}
					record.Value = string(metadataRecordValue)
					newMetaRecord = record
				}
			}
			// Note how for every time the payment is completed, the registration duration increases
			regMetadata.Expiration += uint64(t.RelativeRegistrationExpiration.Microseconds())
			metadataRecordValue, err := json.Marshal(regMetadata)
			if nil != err {
				return nil, err
			}
			newMetaRecord.Value = string(metadataRecordValue)
			nsRecord.Records = []RecordData{newMetaRecord, newZkeyRecord}
			err = t.createOrUpdateRegistration(nsRecord)
			if nil != err {
				return nil, err
			}
			return &regMetadata, nil
		} else {
			// Remove metadata if payment limit exceeded and registration expired
			if time.Now().After(regMetadata.NeedsPaymentUntil) && time.Now().After(time.UnixMicro(int64(regMetadata.Expiration))) {
				t.Logf(LogDebug, "Payment request for `%s' has expired, removing\n", label)
				err := t.expireRegistration(label)
				if nil != err {
					t.Logf(LogInfo, "%s\n", err.Error())
				}
				return nil, nil
			}
		}
	} else {
		if time.Now().After(time.UnixMicro(int64(regMetadata.Expiration))) {
			t.Logf(LogDebug, "Registration for `%s' has expired, removing\n", label)
			err := t.expireRegistration(label)
			if nil != err {
				t.Logf(LogInfo, "%s\n", err.Error())
			}
			return nil, nil
		}
	}
	return &regMetadata, nil
}

func guessDelegationRecordType(val string) string {
	if strings.HasPrefix(val, "000G00") {
		return "PKEY"
	} else {
		return "EDKEY"
	}
}

func isDelegationRecordType(typ string) bool {
	return typ == "PKEY" || typ == "EDKEY"
}

func (t *Registrar) namePage(w http.ResponseWriter, r *http.Request) {
	var (
		namestoreResponse  NamestoreRecord
		value              string
		registeredUntilStr string
		regMetadata        *RegistrationMetadata
		remainingDays      int64
		client             *http.Client
		registered         = r.URL.Query().Get("registered") == "true"
	)
	vars := mux.Vars(r)
	w.Header().Set("Content-Type", "text/html; charset=utf-8")
	label := vars["label"]
	err := t.isNameValid(label)
	sanitizedLabel := url.QueryEscape(vars["label"])
	if nil != err {
		http.Redirect(w, r, fmt.Sprintf("/?error=%s", err), http.StatusSeeOther)
		return
	}
	client = &http.Client{}
	req, _ := http.NewRequest(http.MethodGet, t.GnunetUrl+"/namestore/"+t.RootZoneName+"/"+sanitizedLabel+"?include_maintenance=yes", nil)
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if err != nil {
		http.Redirect(w, r, "/"+"?error=Failed to get zone contents.", http.StatusSeeOther)
		t.Logf(LogError, "Failed to get zone contents\n")
		return
	}
	defer resp.Body.Close()
	if http.StatusOK == resp.StatusCode {
		respData, err := io.ReadAll(resp.Body)
		if err != nil {
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			http.Redirect(w, r, "/"+"?error=Failed to get zone contents.", http.StatusSeeOther)
			return
		}
		err = json.NewDecoder(bytes.NewReader(respData)).Decode(&namestoreResponse)
		if err != nil {
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			http.Redirect(w, r, "/"+"?error=Failed to get zone contents.", http.StatusSeeOther)
			return
		}
		regMetadata, err = t.getCurrentRegistrationMetadata(sanitizedLabel, &namestoreResponse)
		if err != nil {
			t.Logf(LogError, "Failed to get registration metadata: `%s'\n", err.Error())
			http.Redirect(w, r, "/"+"?error=Failed to get registration metadata.", http.StatusSeeOther)
			return
		}
	} else if http.StatusNotFound != resp.StatusCode {
		http.Redirect(w, r, "/"+"?error=Error retrieving zone information.", http.StatusSeeOther)
		return
	}
	for _, record := range namestoreResponse.Records {
		if isDelegationRecordType(record.RecordType) {
			value = record.Value
		}
	}
	if regMetadata != nil {
		if time.Now().Before(time.UnixMicro(int64(regMetadata.Expiration))) {
			registeredUntil := time.UnixMicro(int64(regMetadata.Expiration))
			registeredUntilStr = registeredUntil.Format(time.DateTime)
			remainingDays = int64(time.Until(registeredUntil).Hours() / 24)
		}
	}
	cost, _ := t.RegistrationCost.FormatWithCurrencySpecification(t.CurrencySpec)
	fullData := map[string]any{
		"label":                 sanitizedLabel,
		"version":               t.Cfg.Version,
		"error":                 r.URL.Query().Get("error"),
		"zkey":                  r.URL.Query().Get("zkey"),
		"cost":                  cost,
		"available":             regMetadata == nil,
		"currentValue":          value,
		"suffixHint":            t.SuffixHint,
		"registrationDaysCount": t.RegistrationExpirationDaysCount,
		"registeredUntil":       registeredUntilStr,
		"remainingDays":         remainingDays,
		"registrationSuccess":   registered,
	}
	err = t.NameTpl.Execute(w, fullData)
	if err != nil {
		t.Logf(LogError, "%s\n", err.Error())
	}
}

type LogLevel int

const (
	LogError LogLevel = iota
	LogWarning
	LogInfo
	LogDebug
)

var LoglevelStringMap = map[LogLevel]string{
	LogDebug:   "DEBUG",
	LogError:   "ERROR",
	LogWarning: "WARN",
	LogInfo:    "INFO",
}

func (t *Registrar) Logf(loglevel LogLevel, fmt string, args ...any) {
	if loglevel > t.Cfg.Loglevel {
		return
	}
	t.Logger.SetPrefix("taler-directory - " + LoglevelStringMap[loglevel] + " ")
	t.Logger.Printf(fmt, args...)
}

func (t *Registrar) getFileName(relativeFileName string) string {
	_, err := os.Stat(relativeFileName)
	if errors.Is(err, os.ErrNotExist) {
		_, err := os.Stat(t.Cfg.Datahome + "/" + relativeFileName)
		if errors.Is(err, os.ErrNotExist) {
			t.Logf(LogError, "Tried fallback not found `%s'\n", t.Cfg.Datahome+"/"+relativeFileName)
			return ""
		}
		return t.Cfg.Datahome + "/" + relativeFileName
	}
	return relativeFileName
}

func (t *Registrar) setupHandlers() {
	t.Router = mux.NewRouter().StrictSlash(true)

	t.Router.HandleFunc("/", t.landingPage).Methods("GET")
	t.Router.HandleFunc("/name/{label}", t.namePage).Methods("GET")
	t.Router.HandleFunc("/name/{label}/buy", t.buyPage).Methods("GET")
	t.Router.HandleFunc("/name/{label}/buy/payment", t.paymentPage).Methods("GET")
	t.Router.HandleFunc("/name/{label}/edit", t.editRegistration).Methods("GET")
	t.Router.HandleFunc("/name/{label}/edit", t.updateRegistration).Methods("POST")
	t.Router.HandleFunc("/search", t.searchPage).Methods("GET")

	/* ToS API */
	// t.Router.HandleFunc("/terms", t.termsResponse).Methods("GET")
	// t.Router.HandleFunc("/privacy", t.privacyResponse).Methods("GET")

	/* Config API */
	t.Router.HandleFunc("/config", t.configResponse).Methods("GET")

	/* Assets HTML */
	t.Router.PathPrefix("/css").Handler(http.StripPrefix("/css", http.FileServer(http.Dir(t.getFileName("static/css")))))
	t.Router.PathPrefix("/images").Handler(http.StripPrefix("/images", http.FileServer(http.Dir(t.getFileName("static/images")))))
}

// Initialize the gnsregistrar instance with cfgfile
func (t *Registrar) Initialize(cfg RegistrarConfig) {
	var (
		identityResponse IdentityInfo
		err              error
	)
	t.Cfg = cfg
	t.Logger = log.New(os.Stdout, "gnunet-gns-registrar:", log.LstdFlags)
	if t.Cfg.Ini.Section("gns-registrar").Key("production").MustBool(false) {
		t.Logf(LogInfo, "Production mode enabled\n")
	}
	navTplFile := t.Cfg.Ini.Section("gns-registrar").Key("nav_template").MustString(t.getFileName("web/templates/nav.html"))
	footerTplFile := t.Cfg.Ini.Section("gns-registrar").Key("footer_template").MustString(t.getFileName("web/templates/footer.html"))
	landingTplFile := t.Cfg.Ini.Section("gns-registrar").Key("landing_template").MustString(t.getFileName("web/templates/landing.html"))
	t.LandingTpl, err = template.ParseFiles(landingTplFile, navTplFile, footerTplFile)
	if err != nil {
		t.Logf(LogError, "%s\n", err.Error())
		os.Exit(1)
	}
	nameTplFile := t.Cfg.Ini.Section("gns-registrar").Key("name_template").MustString(t.getFileName("web/templates/name.html"))
	t.NameTpl, err = template.ParseFiles(nameTplFile, navTplFile, footerTplFile)
	if err != nil {
		t.Logf(LogError, err.Error())
		os.Exit(1)
	}
	buyTplFile := t.Cfg.Ini.Section("gns-registrar").Key("buy_template").MustString(t.getFileName("web/templates/buy.html"))
	t.BuyTpl, err = template.ParseFiles(buyTplFile, navTplFile, footerTplFile)
	if err != nil {
		t.Logf(LogError, "%s\n", err.Error())
		os.Exit(1)
	}
	editTplFile := t.Cfg.Ini.Section("gns-registrar").Key("edit_template").MustString(t.getFileName("web/templates/edit.html"))
	t.EditTpl, err = template.ParseFiles(editTplFile, navTplFile, footerTplFile)
	if err != nil {
		t.Logf(LogError, "%s\n", err.Error())
		os.Exit(1)
	}
	paymentExp := t.Cfg.Ini.Section("gns-registrar").Key("payment_required_expiration").MustString("1h")
	recordExp := t.Cfg.Ini.Section("gns-registrar").Key("relative_delegation_expiration").MustString("24h")
	t.RegistrationExpirationDaysCount = t.Cfg.Ini.Section("gns-registrar").Key("registration_duration_days").MustUint64(5)
	t.RelativeRegistrationExpiration, _ = time.ParseDuration(fmt.Sprintf("%dh", t.RegistrationExpirationDaysCount*24))
	t.RelativeDelegationExpiration, _ = time.ParseDuration(recordExp)
	t.PaymentExpiration, _ = time.ParseDuration(paymentExp)
	costStr := t.Cfg.Ini.Section("gns-registrar").Key("registration_cost").MustString("KUDOS:0.3")
	t.RegistrationCost, err = talerutil.ParseAmount(costStr)
	if err != nil {
		t.Logf(LogError, "Error parsing amount `%s': `%s'\n", costStr, err.Error())
		os.Exit(1)
	}
	t.BaseUrl = t.Cfg.Ini.Section("gns-registrar").Key("base_url").MustString("http://localhost:11000")
	t.SuffixHint = t.Cfg.Ini.Section("gns-registrar").Key("suffix_hint").MustString("example.alt")
	t.SummaryTemplateString = t.Cfg.Ini.Section("gns-registrar").Key("order_summary_template").MustString("Registration of `${NAME}' at GNUnet FCFS registrar")
	t.RootZoneName = t.Cfg.Ini.Section("gns-registrar").Key("root_zone_name").MustString("test")
	t.GnunetUrl = t.Cfg.Ini.Section("gns-registrar").Key("base_url_gnunet").MustString("http://localhost:7776")
	t.GnunetBasicAuthEnabled = t.Cfg.Ini.Section("gns-registrar").Key("basic_auth_gnunet_enabled").MustBool(true)
	t.GnunetUsername = t.Cfg.Ini.Section("gns-registrar").Key("basic_auth_gnunet_username").MustString("jdoe")
	t.GnunetPassword = t.Cfg.Ini.Section("gns-registrar").Key("basic_auth_gnunet_password").MustString("secret")
	t.ValidLabelRegex = t.Cfg.Ini.Section("gns-registrar").Key("valid_label_regex").MustString("")
	t.ValidLabelScript = t.Cfg.Ini.Section("gns-registrar").Key("valid_label_script").MustString("")
	client := &http.Client{}
	req, _ := http.NewRequest(http.MethodGet, t.GnunetUrl+"/identity/name/"+t.RootZoneName, nil)
	if t.GnunetBasicAuthEnabled {
		req.SetBasicAuth(t.GnunetUsername, t.GnunetPassword)
	}
	resp, err := client.Do(req)
	if err != nil {
		t.Logf(LogError, "Failed to get zone key. Is gnunet running?\n")
		os.Exit(1)
		return
	}
	defer resp.Body.Close()
	if http.StatusNotFound == resp.StatusCode {
		t.Logf(LogError, "Zone not found.\n")
		os.Exit(1)
	} else if http.StatusOK == resp.StatusCode {
		respData, err := io.ReadAll(resp.Body)
		if err != nil {
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			os.Exit(1)
		}
		err = json.NewDecoder(bytes.NewReader(respData)).Decode(&identityResponse)
		if err != nil {
			t.Logf(LogError, "Failed to get zone contents: `%s'\n", err.Error())
			os.Exit(1)
		}
		t.RootZoneKey = identityResponse.Pubkey
	} else {
		t.Logf(LogError, "Failed to get zone contents\n")
		os.Exit(1)
	}
	t.Merchant = cfg.Merchant
	merchConfig, err := t.Merchant.GetConfig()
	if nil != err {
		t.Logf(LogError, "Failed to get merchant config\n")
		os.Exit(1)
	}
	currencySpec, currencySupported := merchConfig.Currencies[t.RegistrationCost.Currency]
	for !currencySupported {
		t.Logf(LogError, "Currency `%s' not supported by merchant!\n", t.RegistrationCost.Currency)
		os.Exit(1)
	}
	t.CurrencySpec = currencySpec
	t.setupHandlers()
}
